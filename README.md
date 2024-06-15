# OTA-Web-Application
OTA Web Application

# Flask File Upload and JSON Handling Application

## Overview

This is a simple Flask web application that allows users to upload a `.bin` file, post JSON data, and retrieve the uploaded file and JSON data. The application serves the following functionalities:
- File upload: Upload a binary file (`.bin`).
- JSON data posting: Post JSON data which is saved on the server.
- Retrieve the uploaded file and JSON data.

## Features

- Upload a `.bin` file to the server.
- Post JSON data to be stored on the server.
- Retrieve the uploaded binary file.
- Retrieve the stored JSON data.

## Installation

1. **Clone the repository:**
    ```bash
    git clone https://github.com/yourusername/yourrepository.git
    cd yourrepository
    ```

2. **Create a virtual environment:**
    ```bash
    python3 -m venv venv
    ```

3. **Activate the virtual environment:**
    - On Windows:
        ```bash
        venv\Scripts\activate
        ```
    - On macOS/Linux:
        ```bash
        source venv/bin/activate
        ```

4. **Install the dependencies:**
    ```bash
    pip install -r requirements.txt
    ```

5. **Run the Flask application:**
    ```bash
    python app.py
    ```

## Usage

### Uploading a File

1. Open your web browser and go to `http://127.0.0.1:5000/`.
2. Use the form to upload a `.bin` file.
3. The file will be saved to the `uploads` directory with the name `code.bin`.

### Posting JSON Data

1. Send a POST request to `http://127.0.0.1:5000/post-data` with JSON data in the request body.
2. The JSON data will be saved to the `uploads` directory as `version.json`.

### Retrieving the Uploaded File

- To download the uploaded `.bin` file, go to `http://127.0.0.1:5000/firmware`.

### Retrieving the JSON Data

- To download the stored JSON data, go to `http://127.0.0.1:5000/version`.

## File Structure


