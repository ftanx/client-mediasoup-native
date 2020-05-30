import * as firebase from "firebase";
import {
    connect,
    disconnect,
    EventHandler,
    startReceivingAudio,
    startStreamingAudio, startStreamingVideo,
    stopReceivingAudio,
    stopStreamingAudio, stopStreamingVideo
} from "../functions";
import {DatabaseDevice, DatabaseProducer, DatabaseRouter, DatabaseUser} from "../model";

export class Device {
    private readonly user: firebase.User;
    private readonly router: DatabaseRouter;
    private readonly databaseRef: firebase.database.Reference;
    private stageId: string = undefined;
    private globalAudioProducerId: string = undefined;
    private globalVideoProducerId: string = undefined;
    private isReceivingAudio: boolean = false;
    private lastDatabaseSnapshot: DatabaseDevice = undefined;

    constructor(router: DatabaseRouter, user: firebase.User) {
        this.router = router;
        this.user = user;
        // Get id
        this.lastDatabaseSnapshot = this.getDevice();
        this.databaseRef = firebase
            .database()
            .ref("devices")
            .push(this.lastDatabaseSnapshot);
        this.databaseRef
            .onDisconnect()
            .remove()
            .then(() => {
                // Clean up
            });
        this.registerDatabaseEventHandler();
    }

    public connect = () => {
        const eventHandler: EventHandler = async (error: string, event: string, value: any) => {
            console.log(event);
            if (event === "audioProducerAdded") {
                const {producerId} = value;
                if (this.stageId) {
                    this.globalAudioProducerId = await firebase.firestore()
                        .collection("producers")
                        .add({
                            uid: this.user.uid,
                            stageId: this.stageId,
                            routerId: this.router.id,
                            producerId: producerId,
                            kind: "audio",
                            deviceId: this.databaseRef.key
                        } as DatabaseProducer)
                        .then((ref) => ref.id);
                    this.updateDatabase();
                }
            } else if (event === "videoProducerAdded") {
                const {producerId} = value;
                if (this.stageId) {
                    this.globalVideoProducerId = await firebase.firestore()
                        .collection("producers")
                        .add({
                            uid: this.user.uid,
                            stageId: this.stageId,
                            routerId: this.router.id,
                            producerId: producerId,
                            kind: "video",
                            deviceId: this.databaseRef.key
                        } as DatabaseProducer)
                        .then((ref) => ref.id);
                    this.updateDatabase();
                }
            }
        }
        connect("https://" + this.router.domain + ":3020", eventHandler);
    }

    public disconnect = (): boolean => {
        return disconnect();
    }

    public setSendAudio = (enable: boolean) => {
        if (enable === (this.globalAudioProducerId !== undefined))
            return;
        if (enable) {
            if (!startStreamingAudio()) {
                console.error("Could not request streaming audio");
            }
        } else {
            if (!stopStreamingAudio()) {
                console.error("Could not request stop streaming audio");
            } else {
                firebase.firestore()
                    .collection("producers")
                    .doc(this.globalAudioProducerId)
                    .delete();
                this.globalAudioProducerId = undefined;
            }
        }
    }

    public setSendVideo = (enable: boolean) => {
        if (enable === (this.globalVideoProducerId !== undefined))
            return;
        if (enable) {
            if (!startStreamingVideo()) {
                console.error("Could not request streaming video");
            }
        } else {
            if (!stopStreamingVideo()) {
                console.error("Could not request stop streaming video");
            } else {
                firebase.firestore()
                    .collection("producers")
                    .doc(this.globalAudioProducerId)
                    .delete();
                this.globalVideoProducerId = undefined;
            }
        }
    }

    public setReceiveAudio = (enable: boolean) => {
        return this.setReceiveAudioInternal(enable, true);
    }
    private setReceiveAudioInternal = (enable: boolean, updateDatabase: boolean) => {
        if (enable === this.isReceivingAudio)
            return;
        if (enable) {
            if (startReceivingAudio()) {
                this.isReceivingAudio = true;
            } else {
                console.error("Could not stream audio");
                this.isReceivingAudio = false;
            }
        } else {
            if (stopReceivingAudio()) {
                this.isReceivingAudio = false;
            } else {
                console.error("Could not stop streaming audio");
                this.isReceivingAudio = true;
            }
        }
        if (updateDatabase)
            return this.updateDatabase();
    }

    private getDevice = (): DatabaseDevice => {
        return {
            uid: this.user.uid,
            name: "Native",
            canAudio: true,
            canVideo: true,
            sendAudio: (this.globalAudioProducerId !== undefined),
            sendVideo: (this.globalVideoProducerId !== undefined),
            receiveAudio: this.isReceivingAudio,
            receiveVideo: false
        }
    }

    private registerDatabaseEventHandler = () => {
        this.databaseRef
            .on("value", (snapshot: firebase.database.DataSnapshot) => {
                this.lastDatabaseSnapshot = snapshot.val() as DatabaseDevice;
                this.setSendAudio(this.lastDatabaseSnapshot.sendAudio);
                this.setSendVideo(this.lastDatabaseSnapshot.sendVideo);
                this.setReceiveAudioInternal(this.lastDatabaseSnapshot.receiveAudio, false);
            })
        firebase.firestore()
            .collection("users")
            .doc(this.user.uid)
            .onSnapshot((doc: firebase.firestore.DocumentSnapshot) => {
                const u: DatabaseUser = doc.data() as DatabaseUser;
                this.handleStageIdChange(u.stageId);
            })
    }

    private handleStageIdChange = (stageId: string) => {
        this.stageId = stageId;
    }

    private updateDatabase = () => {
        // Register device
        const device: DatabaseDevice = this.getDevice();
        if (device !== this.lastDatabaseSnapshot) {
            this.lastDatabaseSnapshot = device;
            return this.databaseRef.set(this.getDevice());
        }
    }
}
