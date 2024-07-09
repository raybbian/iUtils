import { Text, View } from "react-native";
import ConfigSelector from "./ConfigSelector";
import { useState, useEffect } from "react";
import { StyleSheet } from "react-native";
import { statusType } from './StatusBar.tsx'
import StatusBar from "./StatusBar.tsx";
import FeatureSelector from "./FeatureSelector.tsx";

export default function Canvas() {
  const [config, setConfig] = useState(1);
  const [appleMode, setAppleMode] = useState(5);

  const [status, setStatus] = useState<{ message: string, category: statusType }>({ message: "Device disconnected.", category: "neutral" });
  const [appReady, setAppReady] = useState(false);
  //get configuration on load
  useEffect(() => {
    //fake get configuration request
    setStatus({
      message: "Getting device configuration...",
      category: "warning"
    })
    const timeout = new Promise(r => setTimeout(r, 2000));
    timeout.then(() => {
      setStatus({
        message: `Got device configuration!`,
        category: 'success'
      })
      setConfig(5)
      setAppReady(true);
    });
  }, [])

  return (
    <View style={styles.canvas}>
      <View style={[styles.container]}>
        <Text style={styles.headerText}>Set Configuration</Text>
        <ConfigSelector
          config={config}
          setConfig={setConfig}
          setStatus={setStatus}
          style={styles.selector}
          appReady={appReady}
          appleMode={appleMode}
        />
        <Text style={styles.headerText}>Enable Features</Text>
        <FeatureSelector
          style={styles.features}
          appReady={appReady}
          setStatus={setStatus}
          config={config}
          appleMode={appleMode}
        />
      </View>
      <StatusBar status={status} style={styles.statusBar} />
    </View >
  );
}

const styles = StyleSheet.create({
  canvas: {
    width: 300,
    height: 400,
    flexDirection: "column",
  },
  container: {
    flexDirection: "column",
    paddingTop: 8,
    paddingBottom: 12,
    paddingHorizontal: 16,
    flexGrow: 1
  },
  selector: {
    height: 36,
    width: "100%"
  },
  features: {
    flexGrow: 1
  },
  headerText: {
    flexGrow: 0,
    fontWeight: "500"
  },
  statusBar: {
    height: 40
  }
})
