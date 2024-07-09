import { Pressable, StyleSheet, PlatformColor, StyleProp, Text, ViewStyle, MouseEvent, GestureResponderEvent, View, Button } from "react-native";
import { useState, } from 'react';
import s from './Style'

export default function ConfigButton({ ind, style, onPressed, enabled }: {
  ind: number;
  style?: StyleProp<ViewStyle>;
  onPressed: ((event: GestureResponderEvent) => void);
  enabled: boolean;
}) {
  const [isPressed, setIsPressed] = useState(false);
  const [isHovered, setIsHovered] = useState(false);

  let bgStyle;
  if (!enabled) {
    bgStyle = s.buttonBgDisabled;
  } else if (isPressed) {
    bgStyle = s.buttonBgPress;
  } else if (isHovered) {
    bgStyle = s.buttonBgHover;
  } else {
    bgStyle = s.buttonBg;
  }

  return (
    <Pressable
      style={[style, styles.configButton, bgStyle]}
      onPressIn={() => setIsPressed(true)}
      onPressOut={() => setIsPressed(false)}
      onHoverIn={() => setIsHovered(true)}
      onHoverOut={() => setIsHovered(false)}
      onPress={onPressed}
    >
      <Text>{ind}</Text>
    </Pressable>
  )
}

const styles = StyleSheet.create({
  configButton: {
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
  },
})
