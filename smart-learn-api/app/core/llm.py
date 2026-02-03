from openai import AsyncOpenAI
from app.config import settings

client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)

async def generate_response(context: str, query: str, system_instruction: str = "You are a helpful assistant.", history: list = None) -> str:
    """
    Generate a response from the LLM based on context, query, and history.
    """
    try:
        messages = [{"role": "system", "content": system_instruction}]
        
        # Add history if provided
        if history:
            for msg in history:
                messages.append({"role": msg["role"], "content": msg["content"]})
        
        # Add current context and query
        if context.strip():
            user_content = f"Context for this question:\n{context}\n\nQuestion: {query}"
        else:
            user_content = query
            
        messages.append({"role": "user", "content": user_content})

        response = await client.chat.completions.create(
            model=settings.LLM_MODEL,
            messages=messages,
            max_tokens=100,
            temperature=0.7
        )
        return response.choices[0].message.content
    except Exception as e:
        return f"Error generating response: {str(e)}"
