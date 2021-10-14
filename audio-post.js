var audio_port = null;
var audio_call = 0;

var Init = Module.cwrap('Init', 'number', []);
var Proc = Module.cwrap('Proc', 'number', []);
var Call = Module.cwrap('Call', null, ['number']);


class AsciickerAudio extends AudioWorkletProcessor 
{
    constructor (...args) 
    {
        super(...args);

        audio_port = this.port;

        this.port.onmessage = (e) => 
        {
            HEAPU8.set(e.data, audio_call)
            Call(e.data.length);
        }

        audio_call = Init();
    }

    process (inputs, outputs, parameters) 
    {
        let left = outputs[0][0];
        let right = outputs[0][1];

        let len = 128;

        let ptr = Proc();

        let heap = Module.HEAP16;
        const norm = 1.0/32767.0;
        for (let i=0,j=ptr>>1; i<len; i++,j+=2)
        {
            left[i] = heap[j+0] * norm;
            right[i] = heap[j+1] * norm;
        }

        return true;
    }
}
  
registerProcessor('asciicker-audio', AsciickerAudio);

