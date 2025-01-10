import os
import psycopg2
from psycopg2 import Error

def get_db_connection():
    """Get database connection using environment variables"""
    try:
        conn = psycopg2.connect(
            dbname=os.getenv('DB_NAME'),
            user=os.getenv('DB_USER'),
            password=os.getenv('DB_PASSWORD'),
            host=os.getenv('DB_HOST', 'localhost'),
            port=os.getenv('DB_PORT', '5432')
        )
        return conn
    except Error as e:
        raise ConnectionError(f"Failed to connect to database: {e}")

def get_or_create_node(cursor, as_number):
    """Get or create a node for given AS number"""
    as_id = f"AS{as_number}"
    description = f"Autonomous System {as_number}"
    
    cursor.execute("""
        INSERT INTO nodes (node_id, description)
        VALUES (%s, %s)
        ON CONFLICT (node_id) DO UPDATE SET 
            description = EXCLUDED.description
        RETURNING node_id
    """, (as_id, description))  # Now returning node_id (ASN) instead of name (UUID)    
    return cursor.fetchone()[0]  # Returns ASN
