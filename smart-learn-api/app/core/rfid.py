import json
import threading
import time
from pathlib import Path

from app.config import settings
from app.core.database import get_kb_metadata, list_knowledge_bases

RFID_STORE_PATH = Path(__file__).resolve().parent.parent.parent / "data" / "rfid_cards.json"

_lock = threading.Lock()
_event_lock = threading.Lock()
_event_id_counter = 0
_scan_events: list[dict] = []


def _ensure_store_dir() -> None:
    RFID_STORE_PATH.parent.mkdir(parents=True, exist_ok=True)


def _load_raw() -> dict[str, dict]:
    _ensure_store_dir()
    if not RFID_STORE_PATH.exists():
        return {}
    try:
        with open(RFID_STORE_PATH, encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else {}
    except (json.JSONDecodeError, OSError):
        return {}


def _save_raw(cards: dict[str, dict]) -> None:
    _ensure_store_dir()
    with open(RFID_STORE_PATH, "w", encoding="utf-8") as f:
        json.dump(cards, f, indent=2)


def _normalize_uid(uid: str) -> str:
    return uid.strip().upper()


def _validate_uid(uid: str) -> None:
    if not uid:
        raise ValueError("UID is required")
    if len(uid) < 4 or len(uid) > 16:
        raise ValueError("UID must be 4–16 hexadecimal characters")
    if not all(c in "0123456789ABCDEF" for c in uid):
        raise ValueError("UID must contain only hexadecimal characters (0-9, A-F)")


def _kb_name(kb_id: str) -> str:
    for kb in list_knowledge_bases():
        if kb["id"] == kb_id:
            return kb.get("name") or kb_id
    try:
        meta = get_kb_metadata(kb_id)
        return meta.get("name") or kb_id
    except Exception:
        return kb_id


def list_rfid_cards() -> list[dict]:
    cards = _load_raw()
    result = []
    for uid, entry in cards.items():
        kb_id = entry.get("kb_id", "")
        result.append(
            {
                "uid": uid,
                "kb_id": kb_id,
                "kb_name": _kb_name(kb_id) if kb_id else "",
                "label": entry.get("label", ""),
                "created_at": entry.get("created_at", 0),
                "updated_at": entry.get("updated_at", 0),
            }
        )
    result.sort(key=lambda c: c.get("updated_at", 0), reverse=True)
    return result


def get_rfid_card(uid: str) -> dict | None:
    uid = _normalize_uid(uid)
    cards = _load_raw()
    entry = cards.get(uid)
    if not entry:
        return None
    kb_id = entry.get("kb_id", "")
    return {
        "uid": uid,
        "kb_id": kb_id,
        "kb_name": _kb_name(kb_id) if kb_id else "",
        "label": entry.get("label", ""),
        "created_at": entry.get("created_at", 0),
        "updated_at": entry.get("updated_at", 0),
    }


def upsert_rfid_card(uid: str, kb_id: str, label: str = "") -> dict:
    uid = _normalize_uid(uid)
    _validate_uid(uid)
    if not kb_id or not kb_id.strip():
        raise ValueError("Knowledge Base is required")

    now = int(time.time())
    with _lock:
        cards = _load_raw()
        existing = cards.get(uid, {})
        cards[uid] = {
            "kb_id": kb_id,
            "kb_url": _build_kb_query_url(kb_id),
            "label": label or existing.get("label", ""),
            "created_at": existing.get("created_at", now),
            "updated_at": now,
        }
        _save_raw(cards)

    return get_rfid_card(uid)  # type: ignore[return-value]


def delete_rfid_card(uid: str) -> bool:
    uid = _normalize_uid(uid)
    with _lock:
        cards = _load_raw()
        if uid not in cards:
            return False
        del cards[uid]
        _save_raw(cards)
    return True


def _build_kb_query_url(kb_id: str) -> str:
    base = settings.API_BASE_URL.rstrip("/")
    return f"{base}/api/v1/kb/{kb_id}/query"


def _push_event(
    uid: str,
    assigned: bool,
    kb_id: str | None,
    kb_name: str | None,
    kb_url: str | None = None,
) -> dict:
    global _event_id_counter
    with _event_lock:
        _event_id_counter += 1
        event = {
            "id": _event_id_counter,
            "uid": uid,
            "assigned": assigned,
            "kb_id": kb_id,
            "kb_name": kb_name,
            "kb_url": kb_url,
            "timestamp": int(time.time()),
        }
        _scan_events.append(event)
        if len(_scan_events) > 100:
            _scan_events.pop(0)
        return dict(event)


def record_rfid_scan(uid: str) -> dict:
    uid = _normalize_uid(uid)
    if not uid:
        raise ValueError("UID is required")
    if len(uid) > 16:
        raise ValueError("UID is too long")

    card = get_rfid_card(uid)
    if not card:
        now = int(time.time())
        with _lock:
            cards = _load_raw()
            cards[uid] = {
                "kb_id": "",
                "label": "",
                "created_at": now,
                "updated_at": now,
            }
            _save_raw(cards)
        card = get_rfid_card(uid)

    if card and card.get("kb_id"):
        kb_id = card["kb_id"]
        kb_url = card.get("kb_url") or _build_kb_query_url(kb_id)
        return _push_event(
            uid,
            True,
            kb_id,
            card.get("kb_name") or _kb_name(kb_id),
            kb_url,
        )

    return _push_event(uid, False, None, None, None)


def get_scan_events_since(since_id: int = 0) -> list[dict]:
    with _event_lock:
        return [dict(e) for e in _scan_events if e["id"] > since_id]
