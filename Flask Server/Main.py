#! /usr/bin/python3
from flask import Flask, request, redirect, url_for, send_from_directory, render_template, jsonify
import os
import requests
import json

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
ALLOWED_EXTENSIONS = {'bin'}


app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER


if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)
    

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


@app.route('/', methods=['GET', 'POST'])
def upload_file():
    if request.method == 'POST':
        if 'file' not in request.files:
            return render_template('upload.html', message="No file part")
        file = request.files['file']
        if file.filename == '':
            return render_template('upload.html', message="No selected file")
        if file and allowed_file(file.filename):
            filename = 'code.bin'
            filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            file.save(filepath)
            return render_template('upload.html', message="File successfully uploaded")
    return render_template('upload.html', message="")

    
@app.route('/post-data', methods=['POST'])
def post_data():
    global data
    try:
        data = request.json
        print(f"Received data: {data}")
        with open(os.path.join(app.config['UPLOAD_FOLDER'], 'version.json'), 'w') as version_file:
                json.dump(data, version_file, indent=4)
        return jsonify({"status": "success"}), 200
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"status": "failed", "error": str(e)}), 400
    

@app.route('/version', methods=['GET'])
def get_Version():
    return send_from_directory(app.config['UPLOAD_FOLDER'], 'version.json')


@app.route('/firmware', methods=['GET'])
def get_firmware():
    return send_from_directory(app.config['UPLOAD_FOLDER'], 'code.bin')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)