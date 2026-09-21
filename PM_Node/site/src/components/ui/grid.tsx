import type { ReactNode } from "react";

/** The wireframe's recurring pattern: cells separated by a 1px hairline. */
export function DividerGrid({
  children,
  className = "",
  dark = false,
}: {
  children: ReactNode;
  className?: string;
  dark?: boolean;
}) {
  return (
    <div
      className={`grid gap-px border ${
        dark
          ? "bg-white/20 border-white/20"
          : "bg-divider border-divider"
      } ${className}`}
    >
      {children}
    </div>
  );
}

export function Cell({
  children,
  className = "",
  dark = false,
}: {
  children: ReactNode;
  className?: string;
  dark?: boolean;
}) {
  return (
    <div className={`${dark ? "bg-deep" : "bg-bg"} p-5 ${className}`}>
      {children}
    </div>
  );
}
