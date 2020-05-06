declare module 'bluetooth-socket' {
    import { Duplex, DuplexOptions } from 'stream';
    class BluetoothSocket extends Duplex {
        constructor(fd: number, options?: DuplexOptions);
    }
    export = BluetoothSocket;
}