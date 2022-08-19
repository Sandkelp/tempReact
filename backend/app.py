from flask import Flask, jsonify, request
from flask_cors import CORS
from random import randrange as rand
import ram

app = Flask(__name__)
CORS(app)

@app.route('/', methods = ['GET'])
def ramRanch():
    return jsonify(ram.ranch[rand(0,5)])
if __name__ == "__main__":
    app.run(debug=True)
