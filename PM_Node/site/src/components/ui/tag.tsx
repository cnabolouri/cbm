import type { ReactNode } from "react";

export function Tag({ children }: { children: ReactNode }) {
  return (
    <span className="inline-flex items-center font-mono text-[10px] tracking-[0.06em] uppercase px-2.5 py-1 border border-divider text-muted">
      {children}
    </span>
  );
}
