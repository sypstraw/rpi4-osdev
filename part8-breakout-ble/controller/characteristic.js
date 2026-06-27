import bleno from 'bleno-mac';
import { uIOhook } from 'uiohook-napi';

const BlenoCharacteristic = bleno.Characteristic;

class EchoCharacteristic extends BlenoCharacteristic {
  constructor() {
    super({
      uuid: 'ec0e',
      properties: ['read', 'write', 'notify'],
      value: null
    });

    this._value = Buffer.alloc(0); 
    this._updateValueCallback = null;

    // Allocate tracking buffers internally
    this.buf = Buffer.allocUnsafe(1);
    this.obuf = Buffer.allocUnsafe(1);

    // Screen layout matching properties
    this.scrwidth = 1440;
    this.divisor = this.scrwidth / 100;

    // Start listening for mouse events natively inside the class
    this._setupMouseHooks();
  }

  _setupMouseHooks() {
    uIOhook.on('mousemove', event => {
       this.buf.writeUInt8(Math.round(event.x / this.divisor), 0);

       if (Buffer.compare(this.buf, this.obuf)) {
          this._value = this.buf;
          
          if (this._updateValueCallback) {
             this._updateValueCallback(this._value);
          }
          this.buf.copy(this.obuf);
       }
    });

    uIOhook.start();
  }

  onReadRequest(offset, callback) {
    console.log('EchoCharacteristic - onReadRequest: value = ' + this._value.toString('hex'));
    // Using bleno.Characteristic explicitly avoiding proto issues
    callback(bleno.Characteristic.RESULT_SUCCESS, this._value);
  }

  onWriteRequest(data, offset, withoutResponse, callback) {
    this._value = data;
    console.log('EchoCharacteristic - onWriteRequest: value = ' + this._value.toString('hex'));

    if (this._updateValueCallback) {
      console.log('EchoCharacteristic - onWriteRequest: notifying');
      this._updateValueCallback(this._value);
    }

    callback(bleno.Characteristic.RESULT_SUCCESS);
  }

  onSubscribe(maxValueSize, updateValueCallback) {
    console.log('EchoCharacteristic - onSubscribe: Central device has connected!');
    this._updateValueCallback = updateValueCallback;
  }

  onUnsubscribe() {
    console.log('EchoCharacteristic - onUnsubscribe');
    this._updateValueCallback = null;
  }
}

export default EchoCharacteristic;
