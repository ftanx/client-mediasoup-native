import {
    connect,
    disconnect,
    startReceivingAudio,
    startStreamingAudio,
    stopReceivingAudio,
    stopStreamingAudio
} from "./functions";
import {
    QKeySequence,
    QApplication,
    QMainWindow,
    QMenu,
    QIcon,
    QSystemTrayIcon,
    QAction,
    QWidget,
    FlexLayout,
    QPushButton,
    QPlainTextEdit
} from "@nodegui/nodegui";
import { Dock } from "@nodegui/os-utils";
import {FIREBASE_CONFIG} from "./env";
import * as firebase from "firebase";
import * as path from "path";
const icon = require("../assets/nodegui_white.png");

firebase.initializeApp(FIREBASE_CONFIG);

const win = new QMainWindow();
const trayIcon = new QIcon(path.resolve(__dirname, icon));
const tray = new QSystemTrayIcon();
tray.setIcon(trayIcon);
tray.show();
tray.setToolTip("hello");
const menu = new QMenu();
tray.setContextMenu(menu);

// -------------------
// Quit Action
// -------------------
const quitAction = new QAction();
quitAction.setText("Quit");
quitAction.setIcon(trayIcon);
quitAction.addEventListener("triggered", () => {
    const app = QApplication.instance();
    app.exit(0);
});

// -------------------
// Action with Submenu
// -------------------
const actionWithSubmenu = new QAction();
const subMenu = new QMenu();
const hideDockAction = new QAction();
hideDockAction.setText("hide");
hideDockAction.addEventListener("triggered", () => {
    Dock.hide();
});
//-----
const showDockAction = new QAction();
showDockAction.setText("show");
showDockAction.addEventListener("triggered", () => {
    Dock.show();
});
//-----
subMenu.addAction(hideDockAction);
subMenu.addAction(showDockAction);
actionWithSubmenu.setMenu(subMenu);
actionWithSubmenu.setText("Mac Dock");

// ----------------
// Dock Hide/Show
// ----------------
const hideAction = new QAction();
hideAction.setText("hide window");
hideAction.setShortcut(new QKeySequence("Alt+H"));
hideAction.addEventListener("triggered", () => {
    win.hide();
});
//-----
const showAction = new QAction();
showAction.setText("show window");
showAction.setShortcut(new QKeySequence("Alt+S"));
showAction.addEventListener("triggered", () => {
    win.show();
});

// ----------------------
// Add everything to menu
// ----------------------
menu.addAction(hideAction);
menu.addAction(showAction);
menu.addAction(actionWithSubmenu);
menu.addAction(quitAction);


const rootView = new QWidget();
const rootViewLayout = new FlexLayout();
rootView.setObjectName('rootView');
rootView.setLayout(rootViewLayout);
const fieldset = new QWidget();
const fieldsetLayout = new FlexLayout();
fieldset.setObjectName('fieldset');
fieldset.setLayout(fieldsetLayout);
const emailInput = new QPlainTextEdit();
emailInput.setObjectName('email');
const passwordInput = new QPlainTextEdit();
passwordInput.setObjectName('password');
const loginButton = new QPushButton();
loginButton.setText('Sign in');
rootViewLayout.addWidget(fieldset);
fieldsetLayout.addWidget(emailInput);
fieldsetLayout.addWidget(passwordInput);
fieldsetLayout.addWidget(loginButton);
rootView.setLayout(rootViewLayout);


loginButton.addEventListener('clicked', () => {
    return firebase
        .auth()
        .signInWithEmailAndPassword(emailInput.toPlainText(), passwordInput.toPlainText())
        .then(() => {

        })
        .catch((error) => console.error(error));
});


win.setWindowTitle("Digital stage");
win.resize(400, 700);

win.show();


const qApp = QApplication.instance();
qApp.setQuitOnLastWindowClosed(false); // required so that app doesnt close if we close all windows.

(global as any).win = win; // To prevent win from being garbage collected.
(global as any).systemTray = tray; // To prevent system tray from being garbage collected.



