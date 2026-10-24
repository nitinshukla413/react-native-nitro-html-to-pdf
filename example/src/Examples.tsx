import React, { useEffect, useState } from 'react';
import { Pressable, StyleSheet, Switch, Text, View } from 'react-native';
import ReactNativeBlobUtil from 'react-native-blob-util';
import { NitroPdf, type PageSize } from 'react-native-nitro-html-to-pdf';
import Pdf from 'react-native-pdf';
import { invoiceHtml, receiptHtml } from './samples';

declare const performance: { now(): number };

const DOCUMENTS: Record<string, string> = {
  Receipt: receiptHtml,
  'Invoice · 20 rows': invoiceHtml(20),
  'Invoice · 200 rows': invoiceHtml(200),
};
const PAGE_SIZES: PageSize[] = ['A4', 'Letter', 'Legal'];
// PDF points: 72 pt = 1 inch.
const MARGINS: Record<string, number> = { None: 0, '0.5 in': 36, '1 in': 72 };
const OUTPUTS = ['Base64', 'File'] as const;
type Output = (typeof OUTPUTS)[number];

type Result = {
  id: number;
  uri: string;
  pages: number;
  kb: number;
  header: string;
  ms: number;
};

function Choice<T extends string>({
  label,
  options,
  value,
  onChange,
}: {
  label: string;
  options: readonly T[];
  value: T;
  onChange: (value: T) => void;
}) {
  return (
    <View style={styles.option}>
      <Text style={styles.label}>{label}</Text>
      <View style={styles.chips}>
        {options.map(option => (
          <Pressable
            key={option}
            onPress={() => onChange(option)}
            style={[styles.chip, option === value && styles.chipActive]}
          >
            <Text
              style={[
                styles.chipText,
                option === value && styles.chipTextActive,
              ]}
            >
              {option}
            </Text>
          </Pressable>
        ))}
      </View>
    </View>
  );
}

let nextId = 0;

export function Examples() {
  const [sample, setSample] = useState('Invoice · 20 rows');
  const [pageSize, setPageSize] = useState<PageSize>('A4');
  const [margin, setMargin] = useState('0.5 in');
  const [landscape, setLandscape] = useState(false);
  const [output, setOutput] = useState<Output>('Base64');
  const [result, setResult] = useState<Result>();
  const [error, setError] = useState<string>();

  useEffect(() => {
    let cancelled = false;
    (async () => {
      const start = performance.now();
      const m = MARGINS[margin];
      // Layout and PDF writing run on a background thread; the JS thread stays free.
      const pdf = await NitroPdf.generate({
        html: DOCUMENTS[sample],
        pageSize,
        landscape,
        margins: { top: m, right: m, bottom: m, left: m },
      });
      const ms = performance.now() - start;

      // react-native-pdf displays a base64 data URI or a file.
      let uri: string;
      if (output === 'Base64') {
        uri = `data:application/pdf;base64,${pdf.toBase64()}`;
      } else {
        const path = `${ReactNativeBlobUtil.fs.dirs.CacheDir}/example.pdf`;
        await pdf.saveToFile(path);
        uri = `file://${path}`;
      }

      // Raw bytes without a copy, e.g. for uploads, hashing or storage.
      const bytes = new Uint8Array(pdf.toArrayBuffer(), 0, 8);
      const header = String.fromCharCode(...bytes).trim();

      if (!cancelled) {
        setError(undefined);
        setResult({
          id: nextId++,
          uri,
          pages: pdf.pageCount,
          kb: Math.round(pdf.byteLength / 1024),
          header,
          ms,
        });
      }
    })().catch(e => {
      if (!cancelled) setError(String(e));
    });
    return () => {
      cancelled = true;
    };
  }, [sample, pageSize, margin, landscape, output]);

  return (
    <View style={styles.container}>
      <Choice
        label="Document"
        options={Object.keys(DOCUMENTS)}
        value={sample}
        onChange={setSample}
      />
      <Choice
        label="Page size"
        options={PAGE_SIZES}
        value={pageSize}
        onChange={setPageSize}
      />
      <Choice
        label="Margins"
        options={Object.keys(MARGINS)}
        value={margin}
        onChange={setMargin}
      />
      <Choice
        label="Output for the viewer"
        options={OUTPUTS}
        value={output}
        onChange={setOutput}
      />
      <View style={[styles.option, styles.row]}>
        <Text style={styles.label}>Landscape</Text>
        <Switch value={landscape} onValueChange={setLandscape} />
      </View>

      <Text style={styles.result}>
        {error ??
          (result
            ? `${result.pages} ${result.pages === 1 ? 'page' : 'pages'} · ${result.kb} KB · ` +
              `generated in ${result.ms.toFixed(1)} ms · ${result.header}`
            : 'Generating…')}
      </Text>

      <View style={styles.preview}>
        {result && (
          <Pdf
            key={result.id}
            source={{ uri: result.uri, cache: false }}
            style={styles.pdf}
            trustAllCerts={false}
            onError={e => setError(String(e))}
          />
        )}
      </View>
      <Text style={styles.caption}>Preview by react-native-pdf</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: 'white' },
  option: { marginBottom: 10 },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  label: { fontSize: 13, fontWeight: '600', color: '#555', marginBottom: 6 },
  chips: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  chip: {
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 16,
    borderWidth: 1,
    borderColor: '#ccc',
  },
  chipActive: { backgroundColor: '#2563eb', borderColor: '#2563eb' },
  chipText: { fontSize: 14, color: '#333' },
  chipTextActive: { color: 'white' },
  result: { fontSize: 13, color: '#222', marginVertical: 8 },
  preview: {
    flex: 1,
    borderRadius: 8,
    overflow: 'hidden',
    backgroundColor: '#e5e5e5',
  },
  pdf: { flex: 1, backgroundColor: '#e5e5e5' },
  caption: { fontSize: 11, color: '#888', marginTop: 4, textAlign: 'right' },
});
