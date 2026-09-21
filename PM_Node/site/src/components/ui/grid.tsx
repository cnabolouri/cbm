import type { ReactNode } from "react";

/** The wireframe's recurring pattern: cells separated by a 1px hairline. */
export function DividerGrid({
  children,
  className = "",
  feature = false,
}: {
  children: ReactNode;
  className?: string;
  /** Sits inside an inverted feature band rather than on the page ground. */
  feature?: boolean;
}) {
  return (
    <div
      className={`grid gap-px border ${
        feature
          ? "bg-feature-divider border-feature-divider"
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
  feature = false,
}: {
  children: ReactNode;
  className?: string;
  feature?: boolean;
}) {
  return (
    <div className={`${feature ? "bg-feature" : "bg-bg"} p-5 ${className}`}>
      {children}
    </div>
  );
}
