import asyncio
from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

async def get_embedding(text: str) -> list[float]:
    """
    Generate embedding for a single string.
    """
    text = text.replace("\n", " ")
    response = await client.embeddings.create(
        input=[text],
        model=settings.EMBEDDING_MODEL
    )
    return response.data[0].embedding

async def get_embeddings(texts: list[str]) -> list[list[float]]:
    """
    Generate embeddings for a list of strings.
    """
    processed_texts = [t.replace("\n", " ") for t in texts]
    response = await client.embeddings.create(
        input=processed_texts,
        model=settings.EMBEDDING_MODEL
    )
    return [data.embedding for data in response.data]
