const dangerLabels = {
    0: "SAFE",
    1: "CAUTION",
    2: "SENSITIVE",
    3: "DANGEROUS"
};

async function loadRobots() {

    const response = await fetch('/robots');

    const robots = await response.json();

    const container = document.getElementById('robot-container');

    container.innerHTML = '';

    robots.forEach(robot => {

        const card = document.createElement('div');

        card.className = 'robot-card';

        const formattedTime = robot.last_updated
            ? new Date(robot.last_updated).toLocaleString()
            : 'No updates yet';

        card.innerHTML = `

            <div class="robot-header">
                <div class="robot-name">${robot.name}</div>

                <div class="robot-type">${robot.type}</div>
            </div>

            <div class="info-row">
                <div class="label">Current Task</div>
                <div class="value">${robot.current_task}</div>
            </div>

            <div class="info-row">
                <div class="label">Status</div>

                <div class="status-badge status-${robot.status}">
                    ${robot.status}
                </div>
            </div>

            <div class="info-row">
                <div class="label">Battery</div>

                <div class="value">${robot.battery}%</div>

                <div class="battery-bar">
                    <div class="battery-fill"
                         style="width:${robot.battery}%">
                    </div>
                </div>
            </div>

            <div class="info-row">
                <div class="label">Task Completion</div>

                <div class="value">${robot.completion}%</div>

                <div class="progress-bar">
                    <div class="progress-fill"
                         style="width:${robot.completion}%">
                    </div>
                </div>
            </div>

            <div class="danger-box danger-${robot.danger_level}">
                ${dangerLabels[robot.danger_level]}
            </div>

            <div class="command-section">

                <div class="info-row">
                    <div class="label">Last Command</div>
                    <div class="value">
                        ${robot.last_command || 'None'}
                    </div>
                </div>

                <div class="info-row">
                    <div class="label">Operator</div>
                    <div class="value">
                        ${robot.last_command_by || 'N/A'}
                    </div>
                </div>

                <div class="last-update">
                    Updated: ${formattedTime}
                </div>

            </div>
        `;

        container.appendChild(card);
    });

    document.getElementById('refresh-time').innerText =
        `Last refresh: ${new Date().toLocaleTimeString()}`;
}

loadRobots();

setInterval(loadRobots, 5000);