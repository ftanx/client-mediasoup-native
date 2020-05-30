const addon = require('bindings')('digitalstage');

export type EventHandler = (error: string, event: string, value: any) => void;

export const connect = (url: string, eventHandler: EventHandler): void => {
    const jsonHandler = (error: string | null, text: string) => {
        const json = JSON.parse(text);
        const event: string = json.event;
        const value: any = json.value;
        eventHandler(error, event, value);
    };
    addon.connect(url, jsonHandler);
};

export const disconnect = (): boolean => addon.disconnect();
export const startStreamingAudio = (): boolean => addon.startStreamingAudio();
export const stopStreamingAudio = (): boolean => addon.stopStreamingAudio();
export const startStreamingVideo = (): boolean => addon.startStreamingVideo();
export const stopStreamingVideo = (): boolean => addon.stopStreamingVideo();
export const startReceivingAudio = (): boolean => addon.startReceivingAudio();
export const stopReceivingAudio = (): boolean => addon.stopReceivingAudio();
