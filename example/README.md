# Example app

- **Examples**: pick a document, page size, margins and orientation. The PDF regenerates and is previewed with react-native-pdf, from base64 or a file.
- **Benchmark**: times react-native-nitro-html-to-pdf against react-native-html-to-pdf and expo-print.

## Run

From the repository root:

```sh
npm install
cd example
npm run android
```

For iOS, install pods first:

```sh
cd example/ios && pod install && cd ..
npm run ios
```
