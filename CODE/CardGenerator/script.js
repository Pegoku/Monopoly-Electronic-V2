const CARD_WIDTH_MM = 54;
const CARD_HEIGHT_MM = 85.5;
const SLOTS_PER_PAGE = 9;
const GRID_COLUMNS = 3;
const GRID_ROWS = 3;
const GRID_GAP_MM = 4;

const I18N = {
  en: {
    appTitle: "Monopoly Card Generator",
    appSubtitle: "Print-ready 54 x 85.5 mm cards laid out on A4 for duplex printing.",
    controlsAria: "Generator Controls",
    languageLabel: "Language",
    cardSizeLabel: "Card Size",
    cardSizeValue: "54 x 85.5 mm (NFC standard)",
    mirrorBacksLabel: "Mirror back pages (better for manual flip printing)",
    includeActionDecksLabel: "Include Chance and Community Chest",
    generateBtn: "Generate Layout",
    printBtn: "Print / Save A4 PDF",
    sheetFront: "Front",
    sheetBack: "Back",
    price: "PRICE",
    rent: "Rent",
    with1House: "With 1 House",
    with2Houses: "With 2 Houses",
    with3Houses: "With 3 Houses",
    with4Houses: "With 4 Houses",
    withHotel: "With Hotel",
    houseCost: "House cost",
    each: "each",
    mortgageValue: "Mortgage value",
    if2Railroads: "If 2 Railroads",
    if3Railroads: "If 3 Railroads",
    if4Railroads: "If 4 Railroads",
    utilityBothOwned: "If both Utilities owned",
    diceRoll: "dice roll",
    chance: "Chance",
    communityChest: "Community Chest",
    titleDeed: "Title Deed",
    propertySubtitle: "Monopoly Property Card",
    actionSubtitle: "Monopoly Action Card",
    propertyNumber: "Property",
    rentLevel: "RENT LEVEL",
    rentAmount: "RENT",
    copyrightLine: "© 1935, 2015 Hasbro.",
    player: "Player",
    playerSubtitle: "Monopoly Player Card",
    playerLine1: "Player name:",
    playerLine2: "Color / token:",
    playerLine3: "Starting cash:",
  },
  es: {
    appTitle: "Generador de Cartas Monopoly",
    appSubtitle: "Cartas de 54 x 85.5 mm listas para imprimir en A4 a doble cara.",
    controlsAria: "Controles del generador",
    languageLabel: "Idioma",
    cardSizeLabel: "Tamano de carta",
    cardSizeValue: "54 x 85.5 mm (estandar NFC)",
    mirrorBacksLabel: "Reflejar reversos (mejor para impresion manual por volteo)",
    includeActionDecksLabel: "Incluir Suerte y Caja de comunidad",
    generateBtn: "Generar disposicion",
    printBtn: "Imprimir / Guardar PDF A4",
    sheetFront: "Frente",
    sheetBack: "Reverso",
    price: "PRECIO",
    rent: "Renta",
    with1House: "Con 1 casa",
    with2Houses: "Con 2 casas",
    with3Houses: "Con 3 casas",
    with4Houses: "Con 4 casas",
    withHotel: "Con hotel",
    houseCost: "Costo de casa",
    each: "cada una",
    mortgageValue: "Valor de hipoteca",
    if2Railroads: "Si tienes 2 ferrocarriles",
    if3Railroads: "Si tienes 3 ferrocarriles",
    if4Railroads: "Si tienes 4 ferrocarriles",
    utilityBothOwned: "Si tienes ambas companias",
    diceRoll: "valor de dados",
    chance: "Suerte",
    communityChest: "Caja de comunidad",
    titleDeed: "Titulo de propiedad",
    propertySubtitle: "Carta de propiedad Monopoly",
    actionSubtitle: "Carta de accion Monopoly",
    propertyNumber: "Propiedad",
    rentLevel: "NIVEL DE ALQUILER",
    rentAmount: "ALQUILER",
    copyrightLine: "© 1935, 2015 Hasbro.",
    player: "Jugador",
    playerSubtitle: "Carta de jugador Monopoly",
    playerLine1: "Nombre del jugador:",
    playerLine2: "Color / ficha:",
    playerLine3: "Dinero inicial:",
  },
};

const PROPERTY_NAME_ES = {};

const CHANCE_TEXT = {
  en: [
    "Advance to GO (Collect $200).",
    "Advance to Illinois Avenue. If you pass GO, collect $200.",
    "Advance to St. Charles Place. If you pass GO, collect $200.",
    "Advance token to nearest Utility. If unowned, you may buy it. If owned, roll dice and pay owner ten times amount shown.",
    "Advance token to nearest Railroad. If unowned, you may buy it. If owned, pay owner twice the normal rent.",
    "Advance token to nearest Railroad. If unowned, you may buy it. If owned, pay owner twice the normal rent.",
    "Advance to Boardwalk.",
    "Bank pays you dividend of $50.",
    "Get Out of Jail Free. This card may be kept until needed or traded.",
    "Go Back Three Spaces.",
    "Go to Jail. Go directly to Jail. Do not pass GO. Do not collect $200.",
    "Make general repairs on all your property: for each house pay $25, for each hotel pay $100.",
    "Pay poor tax of $15.",
    "Take a trip to Reading Railroad. If you pass GO, collect $200.",
    "You have been elected Chairman of the Board. Pay each player $50.",
    "Your building loan matures. Collect $150.",
  ],
  es: [
    "Avanza a GO (cobras $200).",
    "Avanza a Avenida Illinois. Si pasas por GO, cobra $200.",
    "Avanza a Plaza St. Charles. Si pasas por GO, cobra $200.",
    "Avanza a la compania de servicios mas cercana. Si no tiene dueno, puedes comprarla. Si tiene dueno, tira dados y paga diez veces el valor.",
    "Avanza al ferrocarril mas cercano. Si no tiene dueno, puedes comprarlo. Si tiene dueno, paga el doble de la renta normal.",
    "Avanza al ferrocarril mas cercano. Si no tiene dueno, puedes comprarlo. Si tiene dueno, paga el doble de la renta normal.",
    "Avanza a Boardwalk.",
    "El banco te paga un dividendo de $50.",
    "Salir de la carcel gratis. Puedes guardar esta carta o negociarla.",
    "Retrocede tres casillas.",
    "Ve a la carcel. Ve directamente a la carcel. No pases por GO. No cobres $200.",
    "Haz reparaciones generales en tus propiedades: paga $25 por cada casa y $100 por cada hotel.",
    "Paga impuesto de pobreza de $15.",
    "Haz un viaje al Ferrocarril Reading. Si pasas por GO, cobra $200.",
    "Has sido elegido presidente de la junta. Paga $50 a cada jugador.",
    "Vence tu prestamo de construccion. Cobra $150.",
  ],
};

const COMMUNITY_TEXT = {
  en: [
    "Advance to GO (Collect $200).",
    "Bank error in your favor. Collect $200.",
    "Doctor's fee. Pay $50.",
    "From sale of stock you get $50.",
    "Get Out of Jail Free. This card may be kept until needed or traded.",
    "Go to Jail. Go directly to Jail. Do not pass GO. Do not collect $200.",
    "Holiday fund matures. Receive $100.",
    "Income tax refund. Collect $20.",
    "It is your birthday. Collect $10 from each player.",
    "Life insurance matures. Collect $100.",
    "Pay hospital fees of $100.",
    "Pay school fees of $50.",
    "Receive $25 consultancy fee.",
    "You are assessed for street repair: $40 per house, $115 per hotel.",
    "You have won second prize in a beauty contest. Collect $10.",
    "You inherit $100.",
  ],
  es: [
    "Avanza a GO (cobras $200).",
    "Error del banco a tu favor. Cobra $200.",
    "Honorarios del medico. Paga $50.",
    "De la venta de acciones obtienes $50.",
    "Salir de la carcel gratis. Puedes guardar esta carta o negociarla.",
    "Ve a la carcel. Ve directamente a la carcel. No pases por GO. No cobres $200.",
    "Vence el fondo vacacional. Recibe $100.",
    "Devolucion de impuestos. Cobra $20.",
    "Es tu cumpleanos. Cobra $10 de cada jugador.",
    "Vence tu seguro de vida. Cobra $100.",
    "Paga gastos de hospital por $100.",
    "Paga cuotas escolares de $50.",
    "Recibe $25 por asesoria.",
    "Te cobran por reparacion de calles: $40 por casa y $115 por hotel.",
    "Ganaste el segundo premio en un concurso de belleza. Cobra $10.",
    "Heredas $100.",
  ],
};

const propertyCards = [
  { type: "property", boardNumber: 1, name: "Ronda de Valencia", color: "#8a5629", price: null, rents: [70, 130, 220, 370, 750] },
  { type: "property", boardNumber: 2, name: "Plaza Lavapiés", color: "#8a5629", price: null, rents: [70, 130, 220, 370, 750] },
  { type: "property", boardNumber: 3, name: "Glorieta Cuatro Caminos", color: "#97cdee", price: null, rents: [80, 140, 240, 410, 800] },
  { type: "property", boardNumber: 4, name: "Avenida Reina Victoria", color: "#97cdee", price: null, rents: [80, 140, 240, 410, 800] },
  { type: "property", boardNumber: 5, name: "Calle Bravo Murillo", color: "#97cdee", price: null, rents: [100, 160, 260, 440, 860] },
  { type: "property", boardNumber: 6, name: "Glorieta de Bilbao", color: "#dd4c9e", price: null, rents: [110, 180, 290, 460, 900] },
  { type: "property", boardNumber: 7, name: "Calle Alberto Aguilera", color: "#dd4c9e", price: null, rents: [110, 180, 290, 460, 900] },
  { type: "property", boardNumber: 8, name: "Calle Fuencarral", color: "#dd4c9e", price: null, rents: [130, 200, 310, 490, 980] },
  { type: "property", boardNumber: 9, name: "Avenida Felipe II", color: "#f2a12f", price: null, rents: [140, 210, 330, 520, 1000] },
  { type: "property", boardNumber: 10, name: "Calle Velázquez", color: "#f2a12f", price: null, rents: [140, 210, 330, 520, 1000] },
  { type: "property", boardNumber: 11, name: "Calle Serrano", color: "#f2a12f", price: null, rents: [160, 230, 350, 550, 1100] },
  { type: "property", boardNumber: 12, name: "Avenida de América", color: "#ee3f39", price: null, rents: [170, 250, 380, 580, 1160] },
  { type: "property", boardNumber: 13, name: "Calle María de Molina", color: "#ee3f39", price: null, rents: [170, 250, 380, 580, 1160] },
  { type: "property", boardNumber: 14, name: "Calle Cea Bermúdez", color: "#ee3f39", price: null, rents: [190, 270, 400, 610, 1200] },
  { type: "property", boardNumber: 15, name: "Avenida de los Reyes Católicos", color: "#efe249", price: null, rents: [200, 280, 420, 640, 1300] },
  { type: "property", boardNumber: 16, name: "Calle Bailén", color: "#efe249", price: null, rents: [200, 280, 420, 640, 1300] },
  { type: "property", boardNumber: 17, name: "Plaza de España", color: "#efe249", price: null, rents: [220, 300, 440, 670, 1340] },
  { type: "property", boardNumber: 18, name: "Puerta del Sol", color: "#36b24a", price: null, rents: [230, 320, 460, 700, 1400] },
  { type: "property", boardNumber: 19, name: "Calle Alcalá", color: "#36b24a", price: null, rents: [230, 320, 460, 700, 1400] },
  { type: "property", boardNumber: 20, name: "Gran Vía", color: "#36b24a", price: null, rents: [250, 340, 480, 730, 1440] },
  { type: "property", boardNumber: 21, name: "Paseo de la Castellana", color: "#356fbb", price: null, rents: [270, 360, 510, 740, 1500] },
  { type: "property", boardNumber: 22, name: "Paseo del Prado", color: "#356fbb", price: null, rents: [300, 400, 560, 810, 1600] },
];

const PLAYER_TOKENS = {
  en: [
    { name: "HAT", iconClass: "fa-hat-cowboy", iconColor: "#f1d06b" },
    { name: "DOG", iconClass: "fa-dog", iconColor: "#ffffff" },
    { name: "CAR", iconClass: "fa-car-side", iconColor: "#d94438" },
    { name: "SHIP", iconClass: "fa-ship", iconColor: "#4f79d9" },
    { name: "CAT", iconClass: "fa-cat", iconColor: "#21bf59" },
    { name: "BOOT", iconClass: "fa-shoe-prints", iconColor: "#f39b34" },
    { name: "THIMBLE", iconClass: "fa-chess-pawn", iconColor: "#6ad8d3" },
    { name: "WHEELBARROW", iconClass: "fa-cart-shopping", iconColor: "#d6c46d" },
  ],
  es: [
    { name: "SOMBRERO", iconClass: "fa-hat-cowboy", iconColor: "#f1d06b" },
    { name: "PERRO", iconClass: "fa-dog", iconColor: "#ffffff" },
    { name: "COCHE", iconClass: "fa-car-side", iconColor: "#d94438" },
    { name: "BARCO", iconClass: "fa-ship", iconColor: "#4f79d9" },
    { name: "GATO", iconClass: "fa-cat", iconColor: "#21bf59" },
    { name: "BOTA", iconClass: "fa-shoe-prints", iconColor: "#f39b34" },
    { name: "DEDAL", iconClass: "fa-chess-pawn", iconColor: "#6ad8d3" },
    { name: "CARRETILLA", iconClass: "fa-cart-shopping", iconColor: "#d6c46d" },
  ],
};

let currentLanguage = localStorage.getItem("monopoly-language") || "es";

function tr(key) {
  return (I18N[currentLanguage] && I18N[currentLanguage][key]) || I18N.en[key] || key;
}

function money(value) {
  return `$${value}`;
}

function cardName(name) {
  if (currentLanguage === "es") return PROPERTY_NAME_ES[name] || name;
  return name;
}

function formatNameTwoLines(name) {
  const words = name.toUpperCase().trim().split(/\s+/);
  if (words.length <= 1) {
    return { line1: words[0] || "", line2: "\u00a0" };
  }

  if (words.length === 2) {
    return { line1: words[0], line2: words[1] };
  }

  let bestBreak = 1;
  let bestDiff = Number.POSITIVE_INFINITY;

  for (let i = 1; i < words.length; i += 1) {
    const left = words.slice(0, i).join(" ");
    const right = words.slice(i).join(" ");
    const diff = Math.abs(left.length - right.length);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestBreak = i;
    }
  }

  return {
    line1: words.slice(0, bestBreak).join(" "),
    line2: words.slice(bestBreak).join(" "),
  };
}

function getAllCards(options = {}) {
  const { includeActionDecks = false } = options;
  const chanceCards = CHANCE_TEXT[currentLanguage] || CHANCE_TEXT.en;
  const communityCards = COMMUNITY_TEXT[currentLanguage] || COMMUNITY_TEXT.en;
  const playerTokens = PLAYER_TOKENS[currentLanguage] || PLAYER_TOKENS.en;

  const buildCardNumber = (id) => {
    const p1 = String(id).padStart(4, "0");
    const p2 = String((1952 + id * 137) % 10000).padStart(4, "0");
    const p3 = String((5231 + id * 211) % 10000).padStart(4, "0");
    const p4 = String((5462 + id * 307) % 10000).padStart(4, "0");
    return `${p1} ${p2} ${p3} ${p4}`;
  };

  const playerCards = playerTokens.map((token, index) => ({
    type: "player",
    deck: "player",
    cardId: index + 1,
    cardNumber: buildCardNumber(index + 1),
    tokenName: token.name,
    tokenIconClass: token.iconClass,
    tokenColor: token.iconColor,
  }));
  const cards = [
    ...propertyCards.map((card) => ({ ...card, deck: "title" })),
    ...playerCards,
  ];

  if (includeActionDecks) {
    cards.push(
      ...chanceCards.map((text, index) => ({ type: "chance", deck: "chance", name: `${tr("chance")} ${index + 1}`, text })),
      ...communityCards.map((text, index) => ({ type: "community", deck: "community", name: `${tr("communityChest")} ${index + 1}`, text }))
    );
  }

  return cards;
}

function createPropertyCard(card) {
  const el = document.createElement("article");
  el.className = "card deed-card property-deed";

  const header = document.createElement("div");
  header.className = "deed-header";
  header.style.setProperty("--deed-color", card.color);
  header.innerHTML = `<span class="deed-number">${card.boardNumber}</span>`;

  const name = document.createElement("div");
  name.className = "deed-name";
  const splitName = formatNameTwoLines(cardName(card.name));
  name.innerHTML = `<span>${splitName.line1}</span><span>${splitName.line2}</span>`;

  const core = document.createElement("div");
  core.className = "deed-core";

  const price = document.createElement("div");
  price.className = "deed-price";
  if (card.price !== null && card.price !== undefined && card.price !== "") {
    price.textContent = `${tr("price")} ${money(card.price)}`;
  }

  const rentTable = document.createElement("div");
  rentTable.className = "deed-rent-table";
  const tableHead = document.createElement("div");
  tableHead.className = "deed-rent-head";
  tableHead.innerHTML = `<span>${tr("rentLevel")}</span><span>${tr("rentAmount")}</span>`;
  const values = card.rents;

  values.forEach((value, idx) => {
    const row = document.createElement("div");
    row.className = "deed-rent-row";
    row.innerHTML = `
      <span class="deed-level" aria-label="${tr("rent")} ${idx + 1}">
        <b class="lvl ${idx === 0 ? "on" : ""}">1</b>
        <b class="lvl ${idx === 1 ? "on" : ""}">2</b>
        <b class="lvl ${idx === 2 ? "on" : ""}">3</b>
        <b class="lvl ${idx === 3 ? "on" : ""}">4</b>
        <b class="lvl ${idx === 4 ? "on" : ""}">5</b>
      </span>
      <strong>${money(value)}</strong>
    `;
    row.style.setProperty("--row-index", `${idx}`);
    rentTable.appendChild(row);
  });

  if (price.textContent) {
    core.append(price);
  }
  core.append(tableHead, rentTable);
  el.append(header, name, core);
  return el;
}

function createSpecialCard(card, kind) {
  const el = document.createElement("article");
  el.className = `card special-card ${kind === "chance" ? "chance-theme" : "chest-theme"}`;

  const head = document.createElement("div");
  head.className = "special-head";
  head.textContent = kind === "chance" ? tr("chance") : tr("communityChest");

  const icon = document.createElement("div");
  icon.className = "special-icon";
  icon.textContent = kind === "chance" ? "?" : "$";

  const text = document.createElement("p");
  text.className = "special-text";
  text.textContent = card.text;

  el.append(head, icon, text);
  return el;
}

function createPlayerCard(card) {
  const el = document.createElement("article");
  el.className = "card player-bank-card";

  const rail = document.createElement("div");
  rail.className = "player-gold-rail";

  const logo = document.createElement("div");
  logo.className = "player-logo-vertical";
  logo.textContent = "MONOPOLY";

  const chip = document.createElement("div");
  chip.className = "player-chip";
  chip.setAttribute("aria-hidden", "true");

  const number = document.createElement("div");
  number.className = "player-bank-number";
  number.textContent = card.cardNumber;

  const token = document.createElement("div");
  token.className = "player-token";
  token.style.color = card.tokenColor;
  token.innerHTML = `<i class="fa-solid ${card.tokenIconClass}" aria-hidden="true"></i>`;

  el.append(rail, logo, chip, number, token);
  return el;
}

function createFrontCard(card) {
  if (card.type === "property") return createPropertyCard(card);
  if (card.type === "player") return createPlayerCard(card);
  if (card.type === "chance") return createSpecialCard(card, "chance");
  return createSpecialCard(card, "community");
}

function createBackCard(card) {
  const el = document.createElement("article");
  el.className = "card card-back";

  const top = document.createElement("div");
  top.className = "back-top";

  const panel = document.createElement("div");
  panel.className = "back-panel";

  if (card.deck === "chance") {
    panel.classList.add("back-panel-chance");
    panel.innerHTML = "<div class=\"back-chance-diamond\"><span class=\"m-main\">M</span><span class=\"m-shadow\">M</span></div>";
  } else if (card.deck === "community") {
    panel.classList.add("back-panel-community");
    panel.innerHTML = "<i class=\"fa-solid fa-house back-house\" aria-hidden=\"true\"></i>";
  } else if (card.deck === "player") {
    panel.classList.add("back-panel-player");
    panel.innerHTML = `<i class=\"fa-solid ${card.tokenIconClass} back-player-icon\" aria-hidden=\"true\"></i>`;
  } else {
    panel.classList.add("back-panel-community");
    panel.innerHTML = "<i class=\"fa-solid fa-house back-house\" aria-hidden=\"true\"></i>";
  }

  el.append(top, panel);
  return el;
}

function createBlankCard() {
  return document.getElementById("blankCardTemplate").content.firstElementChild.cloneNode(true);
}

function positionForBack(slotIndex) {
  const row = Math.floor(slotIndex / GRID_COLUMNS);
  const col = slotIndex % GRID_COLUMNS;
  const mirroredCol = GRID_COLUMNS - 1 - col;
  return row * GRID_COLUMNS + mirroredCol;
}

function fillGrid(gridEl, cards, cardFactory, mirrorBacks = false) {
  const slots = Array(SLOTS_PER_PAGE).fill(null);
  cards.forEach((card, idx) => {
    const slotIndex = mirrorBacks ? positionForBack(idx) : idx;
    slots[slotIndex] = cardFactory(card);
  });
  slots.forEach((slot) => gridEl.appendChild(slot || createBlankCard()));
}

function createSheet(labelText, sideClass) {
  const sheet = document.createElement("section");
  sheet.className = `sheet ${sideClass}`;

  const label = document.createElement("div");
  label.className = "sheet-label";
  label.textContent = labelText;

  const grid = document.createElement("div");
  grid.className = "card-grid";

  sheet.append(label, grid);
  return { sheet, grid };
}

function chunkCards(cards, size) {
  const chunks = [];
  for (let i = 0; i < cards.length; i += size) chunks.push(cards.slice(i, i + size));
  return chunks;
}

function applyStaticTranslations() {
  document.documentElement.lang = currentLanguage;
  document.querySelectorAll("[data-i18n]").forEach((node) => {
    const key = node.getAttribute("data-i18n");
    node.textContent = tr(key);
  });

  document.title = tr("appTitle");
  document.getElementById("controlsSection").setAttribute("aria-label", tr("controlsAria"));
  document.getElementById("cardSizeInput").value = tr("cardSizeValue");
}

function renderSheets() {
  const mirrorBacks = document.getElementById("mirrorBacks").checked;
  const includeActionDecks = document.getElementById("includeActionDecks").checked;
  const container = document.getElementById("sheetContainer");
  const allCards = getAllCards({ includeActionDecks });

  container.innerHTML = "";
  const pages = chunkCards(allCards, SLOTS_PER_PAGE);

  pages.forEach((pageCards, index) => {
    const front = createSheet(`Sheet ${index + 1} - ${tr("sheetFront")}`, "front");
    fillGrid(front.grid, pageCards, createFrontCard, false);
    container.appendChild(front.sheet);

    const back = createSheet(`Sheet ${index + 1} - ${tr("sheetBack")}`, "back");
    fillGrid(back.grid, pageCards, createBackCard, mirrorBacks);
    container.appendChild(back.sheet);
  });
}

function installActions() {
  const renderBtn = document.getElementById("renderBtn");
  const printBtn = document.getElementById("printBtn");
  const languageSelect = document.getElementById("languageSelect");
  const includeActionDecks = document.getElementById("includeActionDecks");

  languageSelect.value = currentLanguage;
  languageSelect.addEventListener("change", (event) => {
    currentLanguage = event.target.value;
    localStorage.setItem("monopoly-language", currentLanguage);
    applyStaticTranslations();
    renderSheets();
  });

  renderBtn.addEventListener("click", renderSheets);
  includeActionDecks.addEventListener("change", renderSheets);
  printBtn.addEventListener("click", () => {
    renderSheets();
    window.print();
  });
}

document.documentElement.style.setProperty("--card-width", `${CARD_WIDTH_MM}mm`);
document.documentElement.style.setProperty("--card-height", `${CARD_HEIGHT_MM}mm`);
document.documentElement.style.setProperty(
  "--grid-width",
  `${GRID_COLUMNS * CARD_WIDTH_MM + (GRID_COLUMNS - 1) * GRID_GAP_MM}mm`
);
document.documentElement.style.setProperty(
  "--grid-height",
  `${GRID_ROWS * CARD_HEIGHT_MM + (GRID_ROWS - 1) * GRID_GAP_MM}mm`
);

applyStaticTranslations();
installActions();
renderSheets();
