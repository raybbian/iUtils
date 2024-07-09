module.exports = function(api) {
  api.cache.never()

  return {
    presets: ['module:@react-native/babel-preset'],
  }
};
