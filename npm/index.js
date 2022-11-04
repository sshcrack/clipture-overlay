const path = require('path');
const bindingPath = path.resolve(path.join(__dirname, '../buildPack', './game_overlay.node'))
const binding = require(bindingPath);

module.exports = binding;
