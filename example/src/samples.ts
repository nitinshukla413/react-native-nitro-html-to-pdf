export const receiptHtml = `
<html><head><style>
  body { font-family: sans-serif; font-size: 14px; }
  .total { font-weight: bold; font-size: 20px; border-top: 1px solid #333; padding-top: 8px; }
</style></head>
<body>
  <h2>Receipt</h2>
  <p>Coffee x2 &mdash; $8.00</p>
  <p>Sandwich &mdash; $6.50</p>
  <p class="total">Total: $14.50</p>
</body></html>`;

export function invoiceHtml(rows: number): string {
  let body = '';
  for (let i = 1; i <= rows; i++) {
    body += `<tr><td>${i}</td><td>Item ${i} with a reasonably long description that wraps onto a second line in narrow columns</td><td class="num">${i * 3}</td><td class="num">$${(i * 12.5).toFixed(2)}</td></tr>`;
  }
  return `
<html><head><title>Invoice</title><style>
  body { font-family: sans-serif; font-size: 12px; color: #222; }
  .header { display: flex; justify-content: space-between; align-items: center; padding: 16px;
            border-radius: 12px; background: linear-gradient(90deg, #1e3a8a, #2563eb); color: white; }
  .badge { border: 2px solid #fff; border-radius: 8px; padding: 4px 10px; font-weight: bold; }
  table { width: 100%; border-collapse: collapse; margin-top: 16px; }
  th { background: #eff6ff; text-align: left; padding: 6px; border-bottom: 2px solid #2563eb; }
  td { padding: 6px; border-bottom: 1px solid #ddd; }
  .num { text-align: right; }
</style></head>
<body>
  <div class="header"><h1>Invoice</h1><span class="badge">PAID</span></div>
  <p>Billed to: <b>Acme Corp</b> &mdash; <i>Net 30</i></p>
  <table><thead><tr><th>#</th><th>Description</th><th class="num">Qty</th><th class="num">Amount</th></tr></thead>
  <tbody>${body}</tbody></table>
</body></html>`;
}
