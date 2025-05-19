from flask import Flask, request, jsonify
import logging

app = Flask(__name__)

# Add the home route for the root URL
@app.route("/")
def home():
    return "Welcome to the Flask app!"

def store_data(sensor, value):
    # Lägg till kod för att lagra data här (t.ex. i en databas)
    pass

@app.route("/store", methods=["POST"])
def store():
    data = request.get_json()
    
    # Kontrollera om nödvändiga data finns
    if "sensor" in data and "value" in data:
        try:
            sensor = data["sensor"]
            value = data["value"]
            
            # Kontrollera om värdet är ett giltigt tal (int eller float)
            if isinstance(value, (int, float)):                         
                store_data(sensor, value)
                return jsonify({"message": "Data stored successfully"}), 201
            else: 
                return jsonify({"error": "Invalid data"}), 400
                           
        except Exception as e: 
            logging.error(f"Error processing the request: {e}")
            return jsonify({"error": "Error processing the request"}), 500
    else:
        return jsonify({"error":"Invalid data, missing 'sensor' or 'value'"}), 400

if __name__ == "__main__":
    # Starta Flask-servern
    app.run(host="0.0.0.0", port=5000, debug=True)
