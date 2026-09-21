import type { ReactNode } from "react";
import { Reveal } from "@/components/reveal";

export function Section({
  eyebrow,
  title,
  kicker,
  feature = false,
  children,
}: {
  eyebrow?: string;
  title: string;
  kicker?: string;
  /** Renders as an inverted band — a deliberate field, in either theme. */
  feature?: boolean;
  children: ReactNode;
}) {
  return (
    <section className={feature ? "bg-feature text-feature-fg" : "bg-bg text-ink"}>
      <div className="max-w-5xl mx-auto px-5 py-16 sm:py-20">
        <Reveal>
          <div className="flex items-baseline justify-between gap-4 flex-wrap mb-8">
            <div>
              {eyebrow && (
                <span
                  className={`block font-mono text-[11px] tracking-[0.12em] uppercase mb-2 ${
                    feature ? "text-feature-accent" : "text-accent-text"
                  }`}
                >
                  {eyebrow}
                </span>
              )}
              <h2 className="text-[28px] sm:text-[32px]">{title}</h2>
            </div>
            {kicker && (
              <span
                className={`font-mono text-[11px] ${
                  feature ? "text-feature-accent" : "text-muted"
                }`}
              >
                {kicker}
              </span>
            )}
          </div>
        </Reveal>
        <Reveal delay={0.08}>{children}</Reveal>
      </div>
    </section>
  );
}
