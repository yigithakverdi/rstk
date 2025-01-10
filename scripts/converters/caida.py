#!/usr/bin/env python3
import os
import sys
from . import utils

def get_weight_from_relationship(relationship):
    relationship = int(relationship)
    if relationship == -1:
        return -1.0  # Provider to customer
    elif relationship == 0:
        return 0.0   # Peer to peer
    else:
        return float(relationship)    

def process_caida_file(input_file):
    """Process CAIDA AS relationship file and store in database"""
    conn = None
    cur = None
    try:
        # Connect to database
        conn = utils.get_db_connection()
        cur = conn.cursor()

        # Process file
        with open(input_file, 'r') as f:
            for line in f:
                # Skip comments and empty lines
                if line.startswith('#') or not line.strip():
                    continue
                
                # Parse line
                parts = line.strip().split('|')
                if len(parts) == 4:
                    as1, as2, relationship, source = parts
                    
                    # Get node IDs
                    as1_id = utils.get_or_create_node(cur, as1)
                    as2_id = utils.get_or_create_node(cur, as2)
                    
                    # Convert relationship to weight
                    weight = get_weight_from_relationship(relationship)
                    
                    # Insert connection
                    cur.execute("""
                        INSERT INTO connections 
                        (source_node_id, target_node_id, weight, relation_type)
                        VALUES (%s, %s, %s, %s)
                        ON CONFLICT (source_node_id, target_node_id) 
                        DO UPDATE SET 
                            weight = EXCLUDED.weight,
                            relation_type = EXCLUDED.relation_type
                    """, (as1_id, as2_id, weight, source))
        
        # Commit changes
        conn.commit()
        print(f"Successfully imported data from {input_file}")

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        if conn:
            conn.rollback()
        sys.exit(1)
    finally:
        if cur:
            cur.close()
        if conn:
            conn.close()

def main():
    if len(sys.argv) != 2:
        print("Usage: python caida.py <input_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    if not os.path.exists(input_file):
        print(f"Error: Input file {input_file} does not exist")
        sys.exit(1)
    
    process_caida_file(input_file)

if __name__ == "__main__":
    main()
