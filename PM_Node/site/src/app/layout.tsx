import type { Metadata, Viewport } from "next";
import { Barlow, Barlow_Condensed, IBM_Plex_Mono } from "next/font/google";
import { SiteHeader } from "@/components/site-header";
import { SiteFooter } from "@/components/site-footer";
import { ThemeProvider } from "@/components/theme-provider";
import "./globals.css";

const barlow = Barlow({
  subsets: ["latin"],
  weight: ["400", "500", "700"],
  variable: "--font-barlow",
});

const barlowCondensed = Barlow_Condensed({
  subsets: ["latin"],
  weight: ["400", "600"],
  variable: "--font-barlow-condensed",
});

const plexMono = IBM_Plex_Mono({
  subsets: ["latin"],
  weight: ["400", "500"],
  variable: "--font-ibm-plex-mono",
});

export const metadata: Metadata = {
  metadataBase: new URL("https://pm-node.sinabolouri.com"),
  title: {
    default: "PM Node — four inspection instruments in one handheld node",
    template: "%s · PM Node",
  },
  description:
    "Vibration, thermal, probe temperature and sound on one ESP32-S3 device, with an on-board TFT, a rotary interface, microSD recording and a local web UI over its own access point.",
  icons: {
    icon: "/logo-icon-color.svg",
  },
};

export const viewport: Viewport = {
  themeColor: [
    { media: "(prefers-color-scheme: light)", color: "#f2f2f3" },
    { media: "(prefers-color-scheme: dark)", color: "#1d2d3d" },
  ],
};

export default function RootLayout({ children }: LayoutProps<"/">) {
  return (
    <html
      lang="en"
      className={`${barlow.variable} ${barlowCondensed.variable} ${plexMono.variable}`}
      suppressHydrationWarning
    >
      <body className="min-h-screen flex flex-col">
        <ThemeProvider>
          <SiteHeader />
          <main className="flex-1">{children}</main>
          <SiteFooter />
        </ThemeProvider>
      </body>
    </html>
  );
}
