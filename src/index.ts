const addon = require('bindings')('digitalstage');


const startSendingAudio = (url: string): Promise<number> => {
    return new Promise<number>((resolve, reject) => {
        addon.start(url, (result) => {
            if (!result)
                reject(result);
            resolve(result);
        });
    })
}

console.log("Hello");

startSendingAudio("https://thepanicure.de:3020")
    .then(result => console.log("RESULT: " + result));

console.log("Finished")