import os
from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO, emit
import librosa
import numpy as np
from sklearn.naive_bayes import GaussianNB
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
import requests

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads/'
socketio = SocketIO(app)

if not os.path.exists(app.config['UPLOAD_FOLDER']):
    os.makedirs(app.config['UPLOAD_FOLDER'])

def extract_features(file_name):
    if not os.path.exists(file_name):
        raise FileNotFoundError(f"File {file_name} tidak ditemukan. Periksa jalur file.")
    audio, sample_rate = librosa.load(file_name, res_type='kaiser_fast')
    mfccs = librosa.feature.mfcc(y=audio, sr=sample_rate, n_mfcc=40)
    mfccs_processed = np.mean(mfccs.T, axis=0)
    return mfccs_processed

audio_files = ['uploads/kotor_1.mp3', 'uploads/kotor_2.mp3', 'uploads/kotor_3.mp3']
labels = ['mengumpat', 'mengumpat', 'mengumpat']

features = []
for file_name in audio_files:
    try:
        data = extract_features(file_name)
        features.append(data)
    except FileNotFoundError as e:
        print(e)

X = np.array(features)
y = np.array(labels)
le = LabelEncoder()
y = le.fit_transform(y)
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
model = GaussianNB()
model.fit(X_train, y_train)

def classify_audio(file_name):
    features = extract_features(file_name)
    features = features.reshape(1, -1)
    prediction = model.predict(features)
    return le.inverse_transform(prediction)[0]

connected_devices = set()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/classify', methods=['POST'])
def classify():
    if 'file' not in request.files:
        return jsonify({'result': 'No file part'})
    file = request.files['file']
    if file.filename == '':
        return jsonify({'result': 'No selected file'})
    if file:
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], file.filename)
        file.save(file_path)
        try:
            result = classify_audio(file_path)
            os.remove(file_path)
            return jsonify({'result': result})
        except Exception as e:
            os.remove(file_path)
            return jsonify({'error': str(e)})

@app.route('/audio', methods=['POST'])
def handle_audio():
    if 'audio' not in request.files:
        return jsonify({'status': 'No audio file'})
    
    audio_file = request.files['audio']
    if audio_file.filename == '':
        return jsonify({'status': 'No selected audio file'})

    device_id = request.headers.get('Device-ID')
    if device_id:
        connected_devices.add(device_id)
    
    file_path = os.path.join(app.config['UPLOAD_FOLDER'], audio_file.filename)
    audio_file.save(file_path)

    try:
        result = classify_audio(file_path)
        if result == 'mengumpat':
            requests.post('http://localhost:5000/audio', json={'device_id': device_id, 'status': 'mengumpat'})
            return jsonify({'status': 'mengumpat', 'device_id': device_id})
        else:
            return jsonify({'status': 'aman', 'device_id': device_id})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)})
    finally:
        if os.path.exists(file_path):
            os.remove(file_path)

@app.route('/device_id', methods=['GET'])
def get_device_id():
    device_id = request.args.get('device_id', 'Unknown')
    return jsonify({'device_id': device_id})

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

@socketio.on('message')
def handle_message(data):
    print('received message: ' + data)
    emit('response', {'data': 'Message received'})

@socketio.on('request_devices')
def handle_request_devices():
    emit('response_devices', list(connected_devices))

if __name__ == '__main__':
    socketio.run(app, port=4000, debug=False)
