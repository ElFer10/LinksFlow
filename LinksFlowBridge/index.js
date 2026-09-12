const indesign = require("indesign");
const app = indesign.app;

const { entrypoints } = require("uxp");

let socket = null;
let reconnectTimer = null;
let shuttingDown = false;

const NS_PHOTOSHOP = "http://ns.adobe.com/photoshop/1.0/";

const RECONNECT_DELAY_MS = 2000;


// ============================================================
// Utilidades de ruta
// ============================================================

async function resolveDocumentPath(document) {
  if (!document)
    return "";

  try {
    const fullName =
      await document.fullName;

    if (fullName && fullName.nativePath) {
      return String(fullName.nativePath);
    }
  } catch (error) {
    console.log(
      "Error obteniendo document.fullName:",
      error
    );
  }

  return "";
}

// ============================================================
// Conexión con LinksFlow
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

  setStatus("Conectando...");

  socket = new WebSocket("ws://127.0.0.1:17321");

  socket.onopen = () => {
    console.log("LinksFlow Bridge: WebSocket abierto");

    setStatus("Identificando InDesign...");

    sendJson({
      version: 1,
      event: "bridgeReady",
      host: "indesign"
    });
  };


  socket.onmessage = (event) => {
    let message = null;

    try {
      message = JSON.parse(event.data);
    } catch (error) {
    }

    if (
      message &&
      message.event ===
      "bridgeRegistered" &&
      message.host === "indesign"
    ) {
      setStatus("Conectado a LinksFlow");

      return;
    }

    handleMessage(
      event.data
    ).catch(
      (error) => {
        console.log("Error procesando mensaje:", error);
      }
    );
  };

  socket.onerror = (event) => {
    console.log("LinksFlow Bridge error", event);
    // onclose gestionará la reconexión.
  };


  socket.onclose = () => {
    console.log("LinksFlow Bridge desconectado");

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
// Utilidades
// ============================================================

function safeValue(
  fn,
  fallback = null
) {
  try {
    const value = fn();

    if (value === undefined) {
      return fallback;
    }

    return value;

  } catch (error) {
    return fallback;
  }
}


function arrayValue(value) {
  if (
    value === null ||
    value === undefined
  ) {
    return null;
  }

  try {
    return Array.from(value);

  } catch (error) {
    return null;
  }
}


function getXmpProperty(
  metadata,
  namespace,
  property
) {
  if (!metadata) {
    return "";
  }

  const value =
    safeValue(
      () =>
        metadata.getProperty(
          namespace,
          property
        ),
      ""
    );

  if (
    value === null ||
    value === undefined
  ) {
    return "";
  }

  return String(value);
}


function extensionFromFileName(
  fileName
) {
  if (!fileName) {
    return "";
  }

  const index =
    fileName.lastIndexOf(".");

  if (index < 0) {
    return "";
  }

  return fileName
    .substring(index + 1)
    .toLowerCase();
}


// ============================================================
// Tipo de archivo
// ============================================================

function normalizeFileType(
  fileName,
  linkType,
  parent
) {
  const extension =
    extensionFromFileName(
      fileName
    );

  switch (extension) {
    case "psd":
    case "psb":
      return "PSD";

    case "tif":
    case "tiff":
      return "TIFF";

    case "jpg":
    case "jpeg":
      return "JPEG";

    case "png":
      return "PNG";

    case "webp":
      return "WebP";

    case "bmp":
      return "BMP";

    case "ai":
      return "AI";

    case "pdf":
      return "PDF";
  }

  const imageTypeName =
    safeValue(
      () => parent.imageTypeName,
      ""
    );

  if (imageTypeName === "TIFF") {
    return "TIFF";
  }

  if (imageTypeName === "JPEG") {
    return "JPEG";
  }

  if (imageTypeName === "PNG") {
    return "PNG";
  }

  if (
    imageTypeName === "Photoshop"
  ) {
    return "PSD";
  }

  if (
    imageTypeName === "Adobe PDF" ||
    String(linkType).includes("PDF")
  ) {
    return "PDF";
  }

  return extension
    ? extension.toUpperCase()
    : String(linkType || "");
}


function isSupportedRasterFormat(
  fileType
) {
  return (
    fileType === "PSD" ||
    fileType === "TIFF" ||
    fileType === "JPEG" ||
    fileType === "PNG" ||
    fileType === "WebP" ||
    fileType === "BMP"
  );
}


// ============================================================
// Estado del enlace
// ============================================================

function determineState(
  linkStatus,
  fileType,
  parent
) {
  if (
    linkStatus ===
    "LINK_MISSING"
  ) {
    return {
      state: "missing",
      message:
        "Archivo no encontrado."
    };
  }

  if (
    linkStatus ===
    "LINK_INACCESSIBLE"
  ) {
    return {
      state: "error",
      message:
        "El enlace no es accesible."
    };
  }

  if (
    linkStatus ===
    "LINK_EMBEDDED"
  ) {
    return {
      state: "unsupported",
      message:
        "El archivo está incrustado en el documento."
    };
  }

  if (
    !isSupportedRasterFormat(
      fileType
    )
  ) {
    return {
      state: "unsupported",
      message:
        fileType
          ? "Formato no compatible: " +
          fileType
          : "Formato no compatible."
    };
  }

  const constructorName =
    safeValue(
      () => parent.constructorName,
      safeValue(
        () => parent.constructor.name,
        ""
      )
    );

  if (
    constructorName !== "Image"
  ) {
    return {
      state: "unsupported",
      message:
        "El enlace no es una imagen raster."
    };
  }

  return {
    state: "ready",
    message: ""
  };
}


// ============================================================
// Perfil ICC
// ============================================================

function getIccProfile(
  link,
  parent
) {
  const metadata =
    safeValue(
      () => link.linkXmp,
      null
    );

  const xmpProfile =
    getXmpProperty(
      metadata,
      NS_PHOTOSHOP,
      "ICCProfile"
    );

  if (xmpProfile) {
    return xmpProfile;
  }

  const parentProfile =
    safeValue(
      () => parent.profile,
      ""
    );

  if (
    parentProfile === null ||
    parentProfile === undefined
  ) {
    return "";
  }

  const profile =
    String(parentProfile);

  if (
    profile === "None" ||
    profile === ""
  ) {
    return "";
  }

  if (
    profile === "Embedded"
  ) {
    return "Embedded";
  }

  return profile;
}


// ============================================================
// Resolución
// ============================================================

function resolutionObject(value) {
  const array =
    arrayValue(value);

  if (
    !array ||
    array.length < 2
  ) {
    return {
      x: 0,
      y: 0
    };
  }

  return {
    x:
      Number(array[0]) || 0,

    y:
      Number(array[1]) || 0
  };
}


// ============================================================
// Análisis de vínculos
// ============================================================

function analyzeLink(link) {
  const parent =
    safeValue(
      () => link.parent,
      null
    );

  const parentOfParent =
    parent
      ? safeValue(
        () => parent.parent,
        null
      )
      : null;

  const fileName =
    safeValue(
      () => link.name,
      ""
    );

  const linkType =
    safeValue(
      () => link.linkType,
      ""
    );

  const linkStatus =
    safeValue(
      () => String(link.status),
      ""
    );

  const fileType =
    normalizeFileType(
      fileName,
      linkType,
      parent
    );

  const stateInfo =
    determineState(
      linkStatus,
      fileType,
      parent
    );

  const page =
    parentOfParent
      ? safeValue(
        () =>
          parentOfParent
            .parentPage
            .name,
        ""
      )
      : "";

  const actualResolution =
    parent
      ? resolutionObject(
        safeValue(
          () => parent.actualPpi,
          null
        )
      )
      : {
        x: 0,
        y: 0
      };

  const effectiveResolution =
    parent
      ? resolutionObject(
        safeValue(
          () => parent.effectivePpi,
          null
        )
      )
      : {
        x: 0,
        y: 0
      };

  let colorMode =
    parent
      ? safeValue(
        () =>
          String(parent.space),
        ""
      )
      : "";

  if (
    colorMode === "undefined" ||
    colorMode === "null"
  ) {
    colorMode = "";
  }

  return {
    linkId:
      safeValue(
        () => link.id,
        0
      ),

    pageItemId:
      parent
        ? safeValue(
          () => parent.id,
          0
        )
        : 0,

    page,

    fileName,

    filePath:
      safeValue(
        () => link.filePath,
        ""
      ),

    fileType,

    colorMode,

    iccProfile:
      parent
        ? getIccProfile(
          link,
          parent
        )
        : "",

    actualResolution,

    effectiveResolution,

    scale: {
      horizontal:
        parent
          ? Number(
            safeValue(
              () =>
                parent
                  .horizontalScale,
              100
            )
          )
          : 100,

      vertical:
        parent
          ? Number(
            safeValue(
              () =>
                parent
                  .verticalScale,
              100
            )
          )
          : 100
    },

    rotation:
      parent
        ? Number(
          safeValue(
            () =>
              parent
                .rotationAngle,
            0
          )
        )
        : 0,

    flip: {
      horizontal: false,
      vertical: false
    },

    state:
      stateInfo.state,

    statusMessage:
      stateInfo.message
  };
}


// ============================================================
// Análisis del documento
// ============================================================

async function analyzeDocument() {

  if (!app) {
    return {
      version: 1,
      success: false,
      error:
        "El objeto app de InDesign no está disponible."
    };
  }

  if (!app.documents) {
    return {
      version: 1,
      success: false,
      error:
        "app.documents no está disponible en InDesign."
    };
  }

  if (
    app.documents.length === 0
  ) {
    return {
      version: 1,
      success: false,
      error:
        "No hay ningún documento abierto."
    };
  }


  if (
    app.documents.length === 0
  ) {
    return {
      version: 1,
      success: false,
      error:
        "No hay ningún documento abierto."
    };
  }

  const document =
    app.activeDocument;

  const links = [];

  for (
    let i = 0;
    i < document.links.length;
    ++i
  ) {
    links.push(
      analyzeLink(
        document.links.item(i)
      )
    );
  }

  const documentPath =
    await resolveDocumentPath(
      document
    );

  return {
    version: 1,
    success: true,

    documentName:
      safeValue(
        () => document.name,
        ""
      ),

    documentPath,

    documentId:
      safeValue(
        () => document.id,
        0
      ),

    links
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

  socket.send(JSON.stringify(value)
  );
}

async function handleMessage(message) {
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

  if (
    request.command === "updateLinks"
  ) {
    try {
      const result =
        await updateLinks(
          request.linkIds || []
        );

      sendJson({
        version: 1,
        id: request.id || "",
        success: true,
        result
      });

    } catch (error) {
      sendJson({
        version: 1,
        id: request.id || "",
        success: false,
        error:
          error && error.message ? error.message : String(error)
      });
    }

    return;
  }

  if (
    request.command ===
    "analyzeDocument"
  ) {
    try {
      const response =
        await analyzeDocument();

      response.id =
        request.id || "";

      sendJson(
        response
      );

    } catch (error) {

      sendJson({
        version: 1,
        id: request.id || "",
        success: false,
        error:
          error &&
            error.message
            ? error.message
            : String(error)
      });
    }

    return;
  }

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
// UI del panel UXP
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
// Entry point UXP
// ============================================================

entrypoints.setup({
  panels: {
    linksFlowBridgePanel: {

      create(rootNode) {
        console.log(
          "LinksFlow Bridge panel creado"
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
          "LinksFlow Bridge panel visible"
        );

        shuttingDown = false;

        connectToLinksFlow();
      },


      hide(rootNode) {
        console.log(
          "LinksFlow Bridge panel oculto"
        );

        //
        // No cerramos el WebSocket.
        //
      },


      destroy(rootNode) {
        console.log(
          "LinksFlow Bridge panel destruido"
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

async function updateLinks(linkIds) {
  if (
    !Array.isArray(linkIds) ||
    linkIds.length === 0
  ) {
    return {
      updated: 0,
      requested: 0,
      details: []
    };
  }

  if (
    !app ||
    !app.documents ||
    app.documents.length === 0
  ) {
    throw new Error(
      "No hay ningún documento abierto."
    );
  }

  const document =
    app.activeDocument;

  const details = [];

  let updated = 0;

  for (const linkId of linkIds) {
    let link = null;

    try {
      link =
        document.links.itemByID(
          Number(linkId)
        );
    } catch (error) {
      details.push({
        linkId,
        success: false,
        reason: "link-not-found"
      });

      continue;
    }

    if (!link) {
      details.push({
        linkId,
        success: false,
        reason: "link-not-found"
      });

      continue;
    }

    //
    // --------------------------------------------------------
    // Esperar a que InDesign detecte que Photoshop
    // modificó el archivo.
    //
    // Normalmente ocurre muy rápido, pero no necesariamente
    // antes de recibir nuestro comando WebSocket.
    // --------------------------------------------------------
    //

    let status = "";

    const maxAttempts = 20;
    const delayMs = 100;

    for (
      let attempt = 0;
      attempt < maxAttempts;
      ++attempt
    ) {
      status =
        safeValue(
          () => String(link.status),
          ""
        );

      if (
        status ===
        "LINK_OUT_OF_DATE"
      ) {
        break;
      }

      await delay(delayMs);
    }

    //
    // Leer nuevamente el estado justo antes
    // de hacer update().
    //

    status =
      safeValue(
        () => String(link.status),
        ""
      );

    //
    // Si está desactualizado, actualizarlo.
    //

    if (
      status ===
      "LINK_OUT_OF_DATE"
    ) {
      try {
        link.update();

        ++updated;

        details.push({
          linkId,
          success: true,
          statusBefore:
            status,
          statusAfter:
            safeValue(
              () =>
                String(link.status),
              ""
            )
        });

      } catch (error) {
        details.push({
          linkId,
          success: false,
          statusBefore:
            status,
          error:
            error &&
              error.message
              ? error.message
              : String(error)
        });
      }

      continue;
    }

    //
    // Si ya está NORMAL, InDesign pudo haber
    // actualizado el vínculo automáticamente.
    //

    if (
      status === "NORMAL"
    ) {
      ++updated;

      details.push({
        linkId,
        success: true,
        statusBefore:
          status,
        statusAfter:
          status,
        reason:
          "already-normal"
      });

      continue;
    }

    //
    // Estado inesperado.
    //

    details.push({
      linkId,
      success: false,
      statusBefore:
        status,
      reason:
        "unexpected-status"
    });
  }

  return {
    requested:
      linkIds.length,

    updated,

    details
  };
}
