from flask import Flask, request, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO
import json

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///data.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = 'I8BinduS3M'  # Replace with your secret key
db = SQLAlchemy(app)
socketio = SocketIO(app)

class Data(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    hr = db.Column(db.String(50))
    spo2 = db.Column(db.String(50))
    status = db.Column(db.String(50))

class ECGData(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    ecg = db.Column(db.Text)  # Store ECG data as a long string of comma-separated values

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        hr = request.form.get('hr')
        spo2 = request.form.get('spo2')
        status = request.form.get('status')
        ecg = request.form.get('ecg')

        # Store data in the database (replace the old record if it exists)
        existing_data = Data.query.first()
        if existing_data:
            existing_data.hr = hr
            existing_data.spo2 = spo2
            existing_data.status = status
        else:
            new_data = Data(hr=hr, spo2=spo2, status=status)
            db.session.add(new_data)

        # Store ECG data
        new_ecg_data = ECGData(ecg=ecg)
        db.session.add(new_ecg_data)

        db.session.commit()

        print(f"Received Hr: {hr}, SpO2: {spo2}, Status: {status}, ECG: {ecg}")  # Debugging print
        socketio.emit('update', {'hr': hr, 'spo2': spo2, 'status': status, 'ecg': ecg})
        return render_template('index.html', hr=hr, spo2=spo2, status=status, ecg=ecg)
    
    # Fetch the latest data from the database
    latest_data = Data.query.first()
    if latest_data:
        hr = latest_data.hr
        spo2 = latest_data.spo2
        status = latest_data.status
    else:
        hr = ""
        spo2 = ""
        status = ""

    # Fetch the latest ECG data
    latest_ecg_data = ECGData.query.order_by(ECGData.id.desc()).first()
    if latest_ecg_data:
        ecg = latest_ecg_data.ecg
    else:
        ecg = ""

    return render_template('index.html', hr=hr, spo2=spo2, status=status, ecg=ecg)

@socketio.on('connect')
def handle_connect():
    print('Client connected')

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    socketio.run(app, debug=True, host="0.0.0.0", port=5001, allow_unsafe_werkzeug=True)
