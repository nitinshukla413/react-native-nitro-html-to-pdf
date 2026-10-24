import React, { useState } from 'react';
import { Pressable, StatusBar, StyleSheet, Text, View } from 'react-native';
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context';
import { Benchmark } from './src/Benchmark';
import { Examples } from './src/Examples';

const TABS = ['Examples', 'Benchmark'] as const;

export default function App() {
  const [tab, setTab] = useState<(typeof TABS)[number]>('Examples');
  return (
    <SafeAreaProvider>
      <StatusBar barStyle="dark-content" />
      <SafeAreaView style={styles.container}>
        <View style={styles.tabs}>
          {TABS.map(name => (
            <Pressable
              key={name}
              onPress={() => setTab(name)}
              style={[styles.tab, tab === name && styles.tabActive]}
            >
              <Text
                style={[styles.tabText, tab === name && styles.tabTextActive]}
              >
                {name}
              </Text>
            </Pressable>
          ))}
        </View>
        {tab === 'Examples' ? <Examples /> : <Benchmark />}
      </SafeAreaView>
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: 'white' },
  tabs: {
    flexDirection: 'row',
    margin: 16,
    marginBottom: 0,
    padding: 3,
    borderRadius: 10,
    backgroundColor: '#f0f0f3',
  },
  tab: { flex: 1, alignItems: 'center', paddingVertical: 8, borderRadius: 8 },
  tabActive: { backgroundColor: 'white' },
  tabText: { fontSize: 14, fontWeight: '600', color: '#666' },
  tabTextActive: { color: '#111' },
});
