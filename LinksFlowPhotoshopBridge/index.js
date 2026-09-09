const { entrypoints } = require("uxp");

let socket = null;
let reconnectTimer = null;
let shuttingDown = false;

const RECONNECT_DELAY_MS = 2000;


// ============================================================
// Reconexión
// ============================================================

function scheduleReconnect() {
  if (shuttingDown) {
    return;
  }

  if (reconnectTimer) {
    return;
  }

  reconnectTimer = setTimeout(
    () => {
      reconnectTimer = null;

      if (!shuttingDown) {
        connectToLinksFlow();
      }
    },
    RECONNECT_DELAY_MS
  );
}


// ============================================================
// Conexión
// ============================================================

function connectToLinksFlow() {
  if (shuttingDown) {
    return;
  }

  if (
    socket &&
    (
      socket.readyState === WebSocket.OPEN ||
      socket.readyState === WebSocket.CONNECTING
    )
  ) {
    return;
  }

  setStatus(
    "Conectando..."
  );

  socket =
    new WebSocket(
      "ws://127.0.0.1:17321"
    );


  socket.onopen = () => {
    console.log(
      "LinksFlow Photoshop Bridge: WebSocket abierto"
    );

    setStatus(
      "Identificando Photoshop..."
    );

    sendJson({
      version: 1,
      event: "bridgeReady",
      host: "photoshop"
    });
  };


  socket.onmessage = (event) => {
    let message = null;

    try {
      message =
        JSON.parse(event.data);
    } catch (error) {
    }

    //
    // Confirmación del registro
    //

    if (
      message &&
      message.event ===
      "bridgeRegistered" &&
      message.host === "photoshop"
    ) {
      console.log(
        "LinksFlow Photoshop Bridge registrado"
      );

      setStatus(
        "Conectado a LinksFlow"
      );

      return;
    }

    handleMessage(
      event.data
    );
  };


  socket.onerror = (event) => {
    console.log(
      "LinksFlow Photoshop Bridge error",
      event
    );

    //
    // onclose se encargará
    // de la reconexión.
    //
  };


  socket.onclose = () => {
    console.log(
      "LinksFlow Photoshop Bridge desconectado"
    );

    socket = null;

    setStatus(
      "Esperando LinksFlow..."
    );

    if (shuttingDown) {
      return;
    }

    scheduleReconnect();
  };
}


// ============================================================
// Protocolo
// ============================================================

function sendJson(value) {
  if (
    !socket ||
    socket.readyState !==
    WebSocket.OPEN
  ) {
    return;
  }

  socket.send(
    JSON.stringify(value)
  );
}


function handleMessage(message) {
  let request;

  try {
    request =
      JSON.parse(message);
  } catch (error) {

    sendJson({
      version: 1,
      success: false,
      error: "Invalid JSON"
    });

    return;
  }

  //
  // Ping
  //

  if (
    request.command === "ping"
  ) {
    sendJson({
      version: 1,
      id: request.id || "",
      success: true,
      result: "pong"
    });

    return;
  }

  //
  // Todavía no hay comandos
  // de procesamiento.
  //

  sendJson({
    version: 1,
    id: request.id || "",
    success: false,
    error:
      "Unknown command: " +
      String(
        request.command || ""
      )
  });
}


// ============================================================
// UI
// ============================================================

function setStatus(text) {
  try {
    const element =
      document.getElementById(
        "linksflow-status"
      );

    if (element) {
      element.textContent =
        text;
    }

  } catch (error) {
  }
}


// ============================================================
// Panel
// ============================================================

entrypoints.setup({
  panels: {

    linksFlowPhotoshopBridgePanel: {

      create(rootNode) {
        console.log(
          "LinksFlow Photoshop Bridge panel creado"
        );

        shuttingDown = false;

        rootNode.innerHTML = `
          <div style="
            padding: 12px;
            font-family: sans-serif;
          ">

            <div style="
              font-weight: bold;
              margin-bottom: 8px;
            ">
              LinksFlow Bridge
            </div>

            <div id="linksflow-status">
              Conectando...
            </div>

          </div>
        `;

        connectToLinksFlow();
      },


      show(rootNode) {
        console.log(
          "LinksFlow Photoshop Bridge panel visible"
        );

        shuttingDown = false;

        connectToLinksFlow();
      },


      hide(rootNode) {
        console.log(
          "LinksFlow Photoshop Bridge panel oculto"
        );

        //
        // No cerramos el socket.
        //
      },


      destroy(rootNode) {
        console.log(
          "LinksFlow Photoshop Bridge panel destruido"
        );

        shuttingDown = true;

        if (reconnectTimer) {
          clearTimeout(
            reconnectTimer
          );

          reconnectTimer = null;
        }

        if (socket) {

          try {
            socket.close();
          } catch (error) {
          }

          socket = null;
        }
      }
    }
  }
});
