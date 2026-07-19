import bleno from 'bleno-mac';
import EchoCharacteristic from './characteristic.js';

var BlenoPrimaryService = bleno.PrimaryService;
const e = new EchoCharacteristic();

console.log('bleno - mouse echo server starting');

bleno.on('stateChange', function(state) {
  console.log('on -> stateChange: ' + state);

  if (state === 'poweredOn') {
    bleno.startAdvertising('echo', ['ec00']);
  } else {
    bleno.stopAdvertising();
  }
});

bleno.on('advertisingStart', function(error) {
  console.log('on -> advertisingStart: ' + (error ? 'error ' + error : 'success'));

  if (!error) {
    bleno.setServices([
      new BlenoPrimaryService({
        uuid: 'ec00',
        characteristics: [e]
      })
    ]);
  }
});
