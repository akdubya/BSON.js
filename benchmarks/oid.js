var ext = require('../lib/bson_ext'),
    pure = require('../lib/bson_pure'),
    sys = require('sys'),
    iter = 100000, i;

sys.puts("------------------------------")
sys.puts("Generating OIDs:");

var startAddon = new Date().getTime();

for (i=0; i<iter; i++) {
  new ext.ObjectID;
}

sys.puts("Addon finished in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

var startPure = new Date().getTime();

for (i=0; i<iter; i++) {
  new pure.ObjectID;
}

sys.puts("Pure finished in " + ((new Date().getTime() - startPure)/1000) + " seconds.");

sys.puts("------------------------------")
sys.puts("Generating OIDs from hex strings:");

startAddon = new Date().getTime();

for (i=0; i<iter; i++) {
  new ext.ObjectID('111111111111111111111111');
}

sys.puts("Addon finished in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

startPure = new Date().getTime();

for (i=0; i<iter; i++) {
  new pure.ObjectID('111111111111111111111111');
}

sys.puts("Pure finished in " + ((new Date().getTime() - startPure)/1000) + " seconds.");

sys.puts("------------------------------")
sys.puts("Calculating timestamp values:");

startAddon = new Date().getTime();

for (i=0; i<iter; i++) {
  new ext.ObjectID().generationTime;
}

sys.puts("Addon finished in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

startPure = new Date().getTime();

for (i=0; i<iter; i++) {
  new pure.ObjectID().generationTime;
}

sys.puts("Pure finished in " + ((new Date().getTime() - startPure)/1000) + " seconds.");

sys.puts("------------------------------")
sys.puts("Convering OIDs to hex strings:");

startAddon = new Date().getTime();

for (i=0; i<iter; i++) {
  new ext.ObjectID().toHexString();
}

sys.puts("Addon finished in " + ((new Date().getTime() - startAddon)/1000) + " seconds.");

startPure = new Date().getTime();

for (i=0; i<iter; i++) {
  new pure.ObjectID().toHexString();
}

sys.puts("Pure finished in " + ((new Date().getTime() - startPure)/1000) + " seconds.");