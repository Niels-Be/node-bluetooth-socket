const BS = require(".");
const s = new BS(0);
s.on("data", console.log);
s.write("Test\n");
setTimeout(()=>{
    s.end("end\n");
}, 3000);