# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set the working directory in the container
WORKDIR /app

# Install system dependencies
# build-essential is often required for compiling some python packages
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy the current directory contents into the container at /app
COPY . /app

# Install any needed packages specified in pyproject.toml
RUN pip install --no-cache-dir .

# Make ports available to the world outside this container
EXPOSE 5000

# Define environment variable
ENV PYTHONUNBUFFERED=1

# Run the application
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "5000"]
