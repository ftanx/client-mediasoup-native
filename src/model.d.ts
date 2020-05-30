export interface DatabaseDevice {
    uid: string;

    name: string;

    ipv4?: string;
    ipv6?: string;

    canAudio: boolean;
    canVideo: boolean;

    sendAudio: boolean;
    sendVideo: boolean;
    receiveAudio: boolean;
    receiveVideo: boolean;
}

export interface DatabaseRouter {
    id: string;
    ipv4: string,
    ipv6: string,
    domain: string;
    port: number,
    slotAvailable: number
}

export interface DatabaseUser {
    uid: string;
    stageId?: string;
}

export interface DatabaseProducer {
    uid: string;
    stageId: string;
    routerId: string;
    kind: string;
    producerId: string;
}
