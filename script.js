const socket = io("http://127.0.0.1:4000");

socket.on("update_table", (data) => {
  updateESPStatus(data.device_id, data.status);
});

const espTables = {};

function addRowToTable(tableId, status = "Aman") {
  const tableBody = document.querySelector(`#${tableId} tbody`);
  const newRow = document.createElement("tr");
  const rowCount = tableBody.rows.length + 1;

  newRow.innerHTML = `
    <td>${rowCount}</td>
    <td contenteditable="true"></td>
    <td class="time">${new Date().toLocaleTimeString()}</td>
    <td>${status === "mengumpat" ? "Mengumpat" : "Aman"}</td>
    <td contenteditable="true" class="esp32-id"></td>
  `;

  tableBody.appendChild(newRow);
  updateRowNumbers(tableId);
}

function updateRowNumbers(tableId) {
  const tableBody = document.querySelector(`#${tableId} tbody`);
  const rows = tableBody.getElementsByTagName("tr");
  for (let i = 0; i < rows.length; i++) {
    rows[i].getElementsByTagName("td")[0].innerText = i + 1;
  }
}

function updateTime(cell) {
  cell.innerText = new Date().toLocaleTimeString();
}

function updateTimeInTable() {
  const tables = document.querySelectorAll(".class-table");
  tables.forEach((table) => {
    const rows = table.querySelectorAll("tbody tr");
    rows.forEach((row) => {
      if (!row.classList.contains("time-stopped")) {
        updateTime(row.getElementsByClassName("time")[0]);
      }
    });
  });
}

function searchClass() {
  const input = document.getElementById("searchInput").value.toLowerCase();
  const tables = document.querySelectorAll(".class-table");

  tables.forEach((table) => {
    const rows = table.querySelectorAll("tbody tr");
    rows.forEach((row) => {
      const cells = row.getElementsByTagName("td");
      let match = false;
      for (let cell of cells) {
        if (cell.innerText.toLowerCase().includes(input)) {
          match = true;
          break;
        }
      }
      row.style.display = match ? "" : "none";
    });
  });
}

function showClass(className) {
  const tables = document.querySelectorAll(".class-table");
  tables.forEach((table) => {
    table.style.display = table.id === className ? "" : "none";
  });
}

function toggleNavbar() {
  const navbar = document.querySelector(".navbar");
  if (navbar) {
    navbar.style.display = navbar.style.display === "flex" ? "none" : "flex";
  }
}

function createTableForESP(espId) {
  if (!espTables[espId]) {
    const container = document.getElementById("dynamicTables");
    const tableId = `espTable_${espId}`;

    const newTable = document.createElement("div");
    newTable.classList.add("class-table");
    newTable.innerHTML = `
      <h1>ESP32 ${espId}</h1>
      <table id="${tableId}">
        <thead>
          <tr>
            <th>Nomor</th>
            <th>Kelas</th>
            <th>Jam</th>
            <th>Status</th>
            <th>ID ESP32</th>
          </tr>
        </thead>
        <tbody></tbody>
      </table>
      <a href="#" class="add-row"><i data-feather="plus"></i></a>
    `;
    container.appendChild(newTable);
    espTables[espId] = tableId;
  }
}

function updateESPStatus(espId, status) {
  createTableForESP(espId);
  const tableId = espTables[espId];
  const tableBody = document.querySelector(`#${tableId} tbody`);
  const rows = tableBody.getElementsByTagName("tr");

  let rowFound = false;
  for (let row of rows) {
    const cells = row.getElementsByTagName("td");
    if (cells[4].innerText === espId) {
      cells[3].innerText = status === "mengumpat" ? "Mengumpat" : "Aman";
      rowFound = true;
      break;
    }
  }

  if (!rowFound) {
    addRowToTable(tableId, status);
    const newRow = tableBody.lastElementChild;
    const cells = newRow.getElementsByTagName("td");
    cells[4].innerText = espId;
    cells[3].innerText = status === "mengumpat" ? "Mengumpat" : "Aman";
  }
}

function handleUnknownESPStatus() {
  for (const espId in espTables) {
    const tableId = espTables[espId];
    const tableBody = document.querySelector(`#${tableId} tbody`);
    const rows = tableBody.getElementsByTagName("tr");

    let espConnected = false;
    for (let row of rows) {
      const cells = row.getElementsByTagName("td");
      if (cells[4].innerText === espId) {
        espConnected = true;
        break;
      }
    }

    if (!espConnected) {
      addRowToTable(tableId, "Unknown");
      const newRow = tableBody.lastElementChild;
      const cells = newRow.getElementsByTagName("td");
      cells[4].innerText = espId;
      cells[3].innerText = "Unknown";
    }
  }
}

setInterval(handleUnknownESPStatus, 10000);

$(document).ready(function () {
  setInterval(() => {
    const clock = document.getElementById("clock");
    if (clock) {
      const now = new Date();
      clock.innerText = now.toLocaleTimeString();
    }
    updateTimeInTable();
  }, 1000);

  $("#classifyForm").on("submit", function (event) {
    event.preventDefault();
    const formData = new FormData(this);

    $.ajax({
      url: "/classify",
      method: "POST",
      data: formData,
      contentType: false,
      processData: false,
      success: function (response) {
        if (response.error) {
          $("#result").text("Error: " + response.error);
        } else {
          $("#result").text("Klasifikasi: " + response.result);
        }
      },
    });
  });

  document.querySelectorAll(".add-row").forEach((button) => {
    button.addEventListener("click", function (event) {
      event.preventDefault();
      const tableId = button.closest(".class-table").querySelector("table").id;
      addRowToTable(tableId);
    });
  });

  document
    .getElementById("navbarToggle")
    .addEventListener("click", toggleNavbar);

  $("#searchInput").on("input", searchClass);
});
