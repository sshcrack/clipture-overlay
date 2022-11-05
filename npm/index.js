const path = require('path');
const bindingPath = path.resolve('./game_overlay.node')
const binding = require(bindingPath) ?? require(path.join(__dirname, "../buildPack/game_overlay.node"));

module.exports = binding;
