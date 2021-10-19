var audio_port = null;
var audio_call = 0;

var Init = Module.cwrap('Init', 'number', ['number']);
var Proc = Module.cwrap('Proc', 'number', []);
var Call = Module.cwrap('Call', null, ['number','number']);
var XOgg = Module.cwrap('XOgg', null, ['number','number','number']);

class AsciickerAudio extends AudioWorkletProcessor 
{
    constructor (...args) 
    {
        super(...args);

        audio_port = this.port;

        this.port.onmessage = (e) => 
        {
            if (e.data.length <= 4096)
            {
                Module.HEAPU8.set(e.data, audio_call)
                Call(audio_call, e.data.length);
            }
            else
            {
                let addr = Module._malloc(e.data.length);
                Module.HEAPU8.set(e.data, addr)
                Call(addr, e.data.length);
                Module._free(addr);
            }
        }

        const c = args[0].processorOptions; // array of {data:Uint8Array, name:Uint8Array}
        let max_size = 0;
        let num = c.length;
        for (const s in c)
            max_size = max_size < c[s].data.length ? c[s].data.length : max_size;

        audio_call = Init(num);

        console.log("PLEASE");

        if (max_size > 0)
        {
            let data = Module._malloc(max_size);
            for (const s in c)
            {
                Module.HEAPU8.set(c[s].name, audio_call);
                Module.HEAPU8[audio_call+c[s].name.length]=0; // terminate name string

                // fill data
                Module.HEAPU8.set(c[s].data, data);

                // call decompressor                
                XOgg(audio_call, data, c[s].data.length);
            }
            Module._free(data);
        }
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

