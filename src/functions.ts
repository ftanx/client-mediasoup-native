const addon = require('bindings')('digitalstage');


export const connect = (url: string): Promise<boolean> => {
    return new Promise<boolean>((resolve, reject) => {
        addon.connect(url, (error, result: boolean) => {
            if (error)
                reject(error);
            resolve(result);
        })
    })
}
export const disconnect = (): Promise<boolean> => {
    return new Promise<boolean>((resolve, reject) => {
        addon.disconnect((error, result: boolean) => {
            if (error)
                reject(error);
            resolve(result);
        });
    })
}
export const startStreamingAudio = (): Promise<string> => {
    return new Promise<string>((resolve, reject) => {
        addon.startStreamingAudio((result) => {
            if (!result)
                reject(result);
            resolve(result);
        });
    })
}
export const stopStreamingAudio = (): Promise<string> => {
    return new Promise<string>((resolve, reject) => {
        addon.stopStreamingAudio((result) => {
            if (!result)
                reject(result);
            resolve(result);
        });
    })
}

export const startReceivingAudio = (): Promise<string> => {
    return new Promise<string>((resolve, reject) => {
        addon.startReceivingAudio((result) => {
            if (!result)
                reject(result);
            resolve(result);
        });
    })
}
export const stopReceivingAudio = (): Promise<string> => {
    return new Promise<string>((resolve, reject) => {
        addon.stopReceivingAudio((result) => {
            if (!result)
                reject(result);
            resolve(result);
        });
    })
}
