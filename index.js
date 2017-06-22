const fs = require('fs');

const BLOCK = 512;
const INT = 2;
const CHAR = 1;

const emptyArr = (size)=> {
  const ret = [];
  for (let i = 0; i < size; i++) ret.push(i);
  return ret;
};

const bufRead = (i => (buf, len)=> {
  i += len;
  return buf.readUIntLE(i - len, len);
})(0);

class Filsys {
  constructor(buf) {
    this.s_isize = bufRead(buf, INT);
    this.s_fsize = bufRead(buf, INT);
    this.s_nfree = bufRead(buf, INT);
    this.s_free = emptyArr(100).map(()=> bufRead(buf, INT));
    this.s_ninode = bufRead(buf, INT);
    this.s_inode = emptyArr(100).map(()=> bufRead(buf, INT));
    this.s_flock = bufRead(buf, CHAR);
    this.s_ilock = bufRead(buf, CHAR);
    this.s_fmod = bufRead(buf, CHAR);
    this.s_ronly = bufRead(buf, CHAR);
    this.s_time = emptyArr(2).map(()=> bufRead(buf, INT));
    this.pad = emptyArr(46).map(()=> bufRead(buf, INT));
  }
  preview() {
    console.log('---- filsys preview ----');
    Object.keys(this).forEach(key => {
      if (typeof this[key] != 'object' || key == 's_time') console.log(key, this[key]);
      else console.log(key, `{${this[key].length}}`);
    });
    console.log('---- filsys preview ----');
  }
}


const disk = fs.readFileSync('v6root');
const next = (i => (size = 0)=> {
  i += size;
  return disk.slice((i - size) * BLOCK, size ? i * BLOCK : undefined);
})(0);

console.log('boot block');
console.log(next(1));
console.log('super block');
const superBlock = next(1);
console.log(superBlock);
const filsys = new Filsys(superBlock);
// console.log(filsys);
filsys.preview();

const inodes = next(filsys.s_isize);
console.log(inodes.length);
