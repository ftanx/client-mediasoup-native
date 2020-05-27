const addon = require('bindings')('digitalstage');


const startSendingAudio = (url: string): Promise<string> => {
    return new Promise<string>((resolve, reject) => {
        addon.start(url, (error, result) => {
            if (error)
                reject(error);
            resolve(result);
        });
    })
}

console.log("Hello");

startSendingAudio("https://thepanicure.de:3020")
    .then(result => console.log(result));