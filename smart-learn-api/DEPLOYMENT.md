# Deployment Guide for AWS EC2

This guide outlines the steps to deploy the `smart-learn-api` application to an AWS EC2 instance using Docker and Docker Compose.

## Prerequisites

1.  **AWS Account**: You need an active AWS account.
2.  **EC2 Key Pair**: A `.pem` file for SSH access to your instance.
3.  **Security Group**: Ensure your EC2 security group allows:
    *   **SSH (Port 22)**: For your IP address.
    *   **HTTP/Custom TCP (Port 8001)**: To access the API (Host Port).
    *   **Custom TCP (Port 5000)**: If utilizing the secondary port.

## Step 1: Launch an EC2 Instance

1.  Go to the AWS Console > EC2 > **Launch Instance**.
2.  **Name**: `smart-learn-api-server`
3.  **OS Image**: Ubuntu Server 22.04 LTS (recommended) or 24.04 LTS.
4.  **Instance Type**: `t2.small` or `t3.small` (t2.micro might be too small for building/running Python containers with dependencies).
5.  **Key Pair**: Select your existing key pair.
6.  **Network Settings**: Check "Allow SSH traffic from Anywhere" (or your IP) and ensure you allow traffic on port `8001` (you might need to add this rule in the Security Group settings after launch or during separate configuration).

## Step 2: Prepare the Server

SSH into your new instance:

```bash
ssh -i /path/to/your-key.pem ubuntu@<your-ec2-public-ip>
```

Update the system and install Docker:

```bash
# Update packages
sudo apt-get update
sudo apt-get upgrade -y

# Install Docker
sudo apt-get install -y docker.io
sudo systemctl start docker
sudo systemctl enable docker

# Add ubuntu user to docker group (avoids using sudo for docker commands)
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt-get install -y docker-compose-plugin
# OR for older versions: sudo apt-get install -y docker-compose

# IMPORTANT: Log out and log back in for group changes to take effect
exit
ssh -i /path/to/your-key.pem ubuntu@<your-ec2-public-ip>
```

## Step 3: Deploy the Application

### Option A: Using Git (Recommended)

1.  Generate a deploy key or use HTTPS to clone your repository.
2.  Clone the repo:
    ```bash
    git clone <your-repo-url>
    cd smart-learn-api
    ```

### Option B: Copy Files Manually (SCP)

From your local machine:
```bash
scp -i /path/to/your-key.pem -r . ubuntu@<your-ec2-public-ip>:~/smart-learn-api
```

## Step 4: Configure Environment

1.  Navigate to the project directory on the server.
2.  Create your `.env` file (copy contents from your local `.env`):
    ```bash
    nano .env
    # Paste your environment variables
    # Ctrl+O, Enter to save. Ctrl+X to exit.
    ```

## Step 5: Start the Application

Run the application using Docker Compose:

```bash
docker compose up -d --build
```

*   `up`: Starts the containers.
*   `-d`: Detached mode (runs in background).
*   `--build`: Forces a build of the image.

## Step 6: Verify Deployment

1.  Check running containers:
    ```bash
    docker compose ps
    ```
2.  View logs:
    ```bash
    docker compose logs -f
    ```
3.  Test the API:
    Open your browser or Postman and visit: `http://<your-ec2-public-ip>:8001` or `http://<your-ec2-public-ip>:8001/docs`.

## Troubleshooting

*   **Deployment fails due to resources**: Try upgrading the EC2 instance type (e.g., from nano/micro to small/medium).
*   **Port 8001 not accessible**: Check the EC2 **Security Group** inbound rules. Ensure Custom TCP for port 8001 is allowed from 0.0.0.0/0.
