#!/bin/bash

# Database connection parameters
export DB_NAME="rstk"
export DB_USER="yigit"
export DB_PASSWORD="password"
export DB_HOST="localhost"
export DB_PORT="5432"

# Optional: Add converter-specific configurations if needed
export CAIDA_SOURCE_TYPE="CAIDA_AS_REL"
export RIPE_SOURCE_TYPE="RIPE_RIS"

# Verify the environment variables are set
echo "Database environment variables set:"
echo "DB_NAME: $DB_NAME"
echo "DB_USER: $DB_USER"
echo "DB_HOST: $DB_HOST"
echo "DB_PORT: $DB_PORT"

# Note: Not showing password in verification
