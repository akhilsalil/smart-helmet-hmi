from flask import Flask, jsonify, request, render_template
import json
from datetime import datetime

app = Flask(__name__)

def load_robots():
    with open('data/robots.json') as f:
        return json.load(f)


def load_tasks():
    with open('data/tasks.json') as f:
        return json.load(f)


def save_tasks(tasks):
    with open('data/tasks.json', 'w') as f:
        json.dump(tasks, f, indent=2)


@app.route('/')
def dashboard():
    return render_template('dashboard.html')


@app.route('/robots', methods=['GET'])
def get_all_robots():

    robots = load_robots()
    tasks = load_tasks()

    merged = []

    for robot in robots:
        task = next(
            (t for t in tasks if t['robot_id'] == robot['id']),
            {}
        )

        merged_robot = {
            **robot,
            **task
        }

        merged.append(merged_robot)
    return jsonify(merged)


@app.route('/robot/<int:robot_id>', methods=['GET'])
def get_robot(robot_id):

    robots = load_robots()
    robot = next(
        (r for r in robots if r['id'] == robot_id),
        None
    )

    if not robot:
        return jsonify({"error": "Robot not found"}), 404

    return jsonify(robot)


@app.route('/robot/<int:robot_id>/status', methods=['GET'])
def get_status(robot_id):

    tasks = load_tasks()
    task = next(
        (t for t in tasks if t['robot_id'] == robot_id),
        None
    )

    if not task:
        return jsonify({"error": "Status not found"}), 404

    return jsonify(task)


@app.route('/robot/<int:robot_id>/command', methods=['POST'])
def send_command(robot_id):

    data = request.json
    command = data.get('command')
    operator = data.get('operator', 'helmet-operator')
    tasks = load_tasks()

    for task in tasks:
        if task['robot_id'] == robot_id:
            task['last_command'] = command
            task['last_command_by'] = operator
            task['last_updated'] = datetime.now().isoformat()
            task['status'] = 'commanded'

    save_tasks(tasks)

    return jsonify({
        "success": True,
        "robot_id": robot_id,
        "command": command
    })


if __name__ == '__main__':
    app.run(
        debug=True,
        host='0.0.0.0',
        port=5000
    )