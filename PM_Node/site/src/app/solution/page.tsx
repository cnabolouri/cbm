import type { Metadata } from "next";
import { Section } from "@/components/section";
import { DividerGrid, Cell } from "@/components/ui/grid";

export const metadata: Metadata = {
  title: "Solution",
  description:
    "PM Node consolidates vibration, thermal, temperature and sound inspection into one ESP32-S3 device.",
};

const PROBLEMS = [
  "Vibration checks",
  "Thermal hotspot detection",
  "Surface / reference temperature",
  "Sound-based anomaly inspection",
  "Local logging and review",
];

const SOLUTION_POINTS = [
  "IIS3DWB-based vibration measurement and FFT analysis",
  "MLX90640 thermal camera display with hotspot and range visualization",
  "DS18B20 probe temperature page with trend history",
  "Sound level and FFT visualization from an I2S microphone",
  "Rotary-encoder-driven page navigation and record toggle",
  "microSD recording",
  "Wi-Fi access point and local web interface for live views",
];

export default function SolutionPage() {
  return (
    <>
      <Section eyebrow="The problem" title="Five tools, five walks to the machine" kicker="01">
        <DividerGrid className="sm:grid-cols-3 lg:grid-cols-5">
          {PROBLEMS.map((label, i) => (
            <Cell key={label} className="grid gap-2">
              <span className="font-mono text-[10px] text-accent-text">
                {String(i + 1).padStart(2, "0")}
              </span>
              <span className="font-heading text-[19px]">{label}</span>
            </Cell>
          ))}
        </DividerGrid>
        <p className="mt-8 max-w-[64ch] text-ink-soft">
          Industrial inspection often requires multiple separate tools for
          each of these. That creates workflow friction, inconsistent data
          capture, and more time spent switching devices than analysing
          machine condition — and the captures never line up.
        </p>
      </Section>

      <Section eyebrow="The solution" title="One node, one workflow" kicker="02" feature>
        <p className="max-w-[64ch] text-feature-muted mb-8">
          PM Node consolidates these inspection functions into one
          ESP32-S3-based embedded device:
        </p>
        <DividerGrid feature className="sm:grid-cols-2">
          {SOLUTION_POINTS.map((point) => (
            <Cell key={point} feature className="text-[15px]">
              {point}
            </Cell>
          ))}
        </DividerGrid>
      </Section>
    </>
  );
}
