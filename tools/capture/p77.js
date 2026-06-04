const fs=require('fs');for(const ln of fs.readFileSync(process.argv[2],'utf8').trim().split('\n')){const o=JSON.parse(ln);if(o.frame&&o.op===77){const p=Buffer.from(o.hex,'hex');
 console.log('len',p.length,'guid@0=',p.readInt32LE(0),'x@4=',p.readFloatLE(4).toFixed(1),'y@8=',p.readFloatLE(8).toFixed(1),'u32@12=',p.readUInt32LE(12));
 console.log('first48hex:',o.hex.slice(0,96)); break;}}
