import { type PageProps } from "$fresh/server.ts";
export default function App({ Component }: PageProps) {
  return (
    <html lang="en">
      <head>
        <meta charset="utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>Paladin in Violent Winds</title>
        <link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css" />
      </head>
      <body>
        <main class="w3-main w3-container">
          <div class="w3-content">
            <Component />
          </div>
        </main>
      </body>
    </html>
  );
}
