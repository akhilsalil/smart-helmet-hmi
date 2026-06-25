const dangerLabels = {
    0: "SAFE",
    1: "CAUTION",
    2: "SENSITIVE",
    3: "DANGER"
};

let robotCache = [];

async function loadRobots() {

    const response = await fetch('/robots');
    const robots = await response.json();

    robotCache = robots;

    const container = document.getElementById('robot-container');
    container.innerHTML = '';

    robots.forEach(robot => {

        const card = document.createElement('div');

        const typeClass = robot.type === "Arduino bot" ? "type-arduino"
            : robot.type === "Dobot" ? "type-dobot"
            : "type-demo";

        const dangerClass = `danger-level-${robot.danger_level}`;

        const batteryClass = robot.battery < 20 ? "battery-fill battery-low" : "battery-fill";

        card.className = `robot-card ${dangerClass}`;

        const formattedTime = robot.last_updated
            ? new Date(robot.last_updated).toLocaleString()
            : 'No updates yet';

        card.innerHTML = `

            <div class="robot-header">
                <div class="robot-name">${robot.name}</div>
                <div class="robot-type ${typeClass}">${robot.type}</div>
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
            </div>
            <div class="battery-bar">
                <div class="${batteryClass}"
                     style="width:${robot.battery}%">
                </div>
            </div>

            <div class="info-row">
                <div class="label">Task Completion</div>
                <div class="value">${robot.completion}%</div>
            </div>
            <div class="progress-bar">
                <div class="progress-fill"
                     style="width:${robot.completion}%">
                </div>
            </div>

            <div class="danger-box danger-${robot.danger_level}">
                ${dangerLabels[robot.danger_level]}
            </div>

            <div class="info-row">
                <div class="label">Capabilities</div>

                <div class="value capabilities">
                    ${
                        robot.capabilities
                            ? robot.capabilities.map(cap =>
                                `<span class="capability-tag">${cap}</span>`
                            ).join('')
                            : 'N/A'
                    }
                </div>
            </div>

            <div class="command-controls">
                <button
                    class="command-button"
                    onclick="openCommandModal(${robot.id})">

                    Command

                </button>
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

function openCommandModal(robotId) {

    const robot = robotCache.find(
        r => r.id === robotId
    );

    if (!robot) {
        return;
    }

    const modal = document.getElementById('command-modal');
    const title = document.getElementById('modal-robot-name');
    const capabilityList = document.getElementById('capability-list');

    title.textContent = robot.name;

    capabilityList.innerHTML = '';

    if (robot.capabilities) {

        robot.capabilities.forEach(capability => {

            const button = document.createElement('button');

            button.className = 'capability-button';

            button.textContent = capability;

            button.onclick = () => {
                sendCommand(
                    robot.id,
                    capability
                );
            };

            capabilityList.appendChild(button);
        });
    }

    modal.classList.remove('hidden');
}

async function sendCommand(robotId, command) {

    try {

        const response = await fetch(
            `/robot/${robotId}/command`,
            {
                method: 'POST',

                headers: {
                    'Content-Type': 'application/json'
                },

                body: JSON.stringify({
                    command: command,
                    operator: 'dashboard-manager'
                })
            }
        );

        const result = await response.json();

        console.log(result);

        closeModal();

        animateCommandProgress(robotId);

    } catch (error) {

        console.error(error);

        alert('Failed to send command');
    }
}

function animateCommandProgress(robotId) {

    const cards = document.querySelectorAll('.robot-card');
    let targetBar = null;

    cards.forEach(card => {
        const btn = card.querySelector('.command-button');
        if (btn && btn.getAttribute('onclick') === `openCommandModal(${robotId})`) {
            targetBar = card.querySelector('.progress-fill');
        }
    });

    if (!targetBar) return;

    targetBar.style.transition = 'none';
    targetBar.style.width = '0%';

    requestAnimationFrame(() => {
        requestAnimationFrame(() => {
            targetBar.style.transition = 'width 8s linear';
            targetBar.style.width = '100%';
        });
    });

    setTimeout(() => {
        targetBar.style.transition = 'none';
    }, 8100);
}

function closeModal() {

    document
        .getElementById('command-modal')
        .classList
        .add('hidden');
}

document
    .getElementById('close-modal')
    .addEventListener(
        'click',
        closeModal
    );

loadRobots();

setInterval(
    loadRobots,
    5000
);