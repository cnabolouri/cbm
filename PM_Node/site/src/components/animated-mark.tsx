export function AnimatedMark({
  size = 64,
  color = "#5980a6",
  className = "",
}: {
  size?: number;
  color?: string;
  className?: string;
}) {
  return (
    <svg
      className={`pmloop ${className}`}
      viewBox="0 0 64 64"
      width={size}
      height={size}
      style={{ color }}
      role="img"
      aria-label="PM Node sensing loop"
    >
      <g fill="none" stroke="currentColor" strokeWidth={5}>
        <g className="ring r1">
          <path d="M26.26 23.81A10 10 0 0 0 26.26 40.19" />
          <path d="M37.74 23.81A10 10 0 0 1 37.74 40.19" />
        </g>
        <g className="ring r2">
          <path d="M21.68 17.25A18 18 0 0 0 21.68 46.75" />
          <path d="M42.32 17.25A18 18 0 0 1 42.32 46.75" />
        </g>
        <g className="ring r3">
          <path d="M17.09 10.70A26 26 0 0 0 17.09 53.30" />
          <path d="M46.91 10.70A26 26 0 0 1 46.91 53.30" />
        </g>
      </g>
      <circle className="core" cx={32} cy={32} r={5} fill="currentColor" />
    </svg>
  );
}
