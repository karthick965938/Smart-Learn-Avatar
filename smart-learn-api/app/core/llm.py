from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

async def generate_response(context: str, query: str, system_instruction: str = "You are a helpful assistant.") -> str:
    """
    Generate a response from the LLM based on context and query.
    """
    try:
        response = await client.chat.completions.create(
            model=settings.LLM_MODEL,
            messages=[
                {"role": "system", "content": system_instruction},
                {"role": "user", "content": f"Context:\n{context}\n\nQuestion: {query}"}
            ],
            max_tokens=500,
            temperature=0.7
        )
        return response.choices[0].message.content
    except Exception as e:
        return f"Error generating response: {str(e)}"
