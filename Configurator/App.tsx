import React from 'react';
import { SafeAreaView } from 'react-native';
import Canvas from './src/Canvas';

function App(): React.JSX.Element {
  return (
    <SafeAreaView>
      <Canvas />
    </SafeAreaView>
  );
}

export default App;
