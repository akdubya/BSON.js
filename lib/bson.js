try {
  module.exports = require('./bson_ext');
} catch(err) {
  if (err.message.indexOf("Cannot find module") !== -1) {
    module.exports = require('./bson_pure');
  } else {
    throw(err);
  }
}