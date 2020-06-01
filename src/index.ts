import {FIREBASE_CONFIG} from "./env";
import * as firebase from "firebase";
import {Device} from "./models/Device";
import {DatabaseRouter} from "./model";

firebase.initializeApp(FIREBASE_CONFIG);


firebase
    .auth()
    .signInWithEmailAndPassword("test@digital-stage.org", "testtesttest")
    .then((credentials: firebase.auth.UserCredential) => credentials.user)
    .then(async (user: firebase.User) => {
        // Get router
        //TODO: Replace with get fastest router implementation
        const router: DatabaseRouter = await firebase.database()
            .ref("routers")
            .once("child_added")
            .then((snapshot) => snapshot.val() as DatabaseRouter);   // Very quick and dirty :P
        const device: Device = new Device(router, user);
        device.connect();
        setTimeout(() => {
            device.setSendAudio(true);
        }, 3000);
        setTimeout(() => {
            device.setSendVideo(true);
        }, 20000);
    })
    .catch((error) => console.error(error));
