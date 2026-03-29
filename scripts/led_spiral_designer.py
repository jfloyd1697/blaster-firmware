import math
import tkinter as tk
from tkinter import filedialog, messagebox

import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.patches import Circle

import trimesh


# ==========================================
# Geometry helpers
# ==========================================
def polar(r, theta):
    return np.array([r * math.cos(theta), r * math.sin(theta)], dtype=float)


def normalize(v):
    n = np.linalg.norm(v)
    if n < 1e-9:
        return np.array([0.0, 0.0], dtype=float)
    return v / n


def arch_spiral_points(r_start, r_end, theta0, theta1, n=1200):
    thetas = np.linspace(theta0, theta1, n)
    b = (r_start - r_end) / (theta1 - theta0)
    pts = []
    for th in thetas:
        r = r_start - b * (th - theta0)
        pts.append(polar(r, th))
    return np.array(pts, dtype=float)


def make_ring_path(center_radius, slit_width, slit_angle_deg, n=500):
    circumference = 2.0 * math.pi * center_radius
    slit_angle = (slit_width / circumference) * 2.0 * math.pi

    start = math.radians(slit_angle_deg) + slit_angle / 2.0
    end = math.radians(slit_angle_deg) + 2.0 * math.pi - slit_angle / 2.0

    thetas = np.linspace(start, end, n)
    return np.array([polar(center_radius, th) for th in thetas], dtype=float)


def compute_path_normals(points):
    pts = np.asarray(points, dtype=float)
    n = len(pts)
    normals = []

    for i in range(n):
        if i == 0:
            tangent = normalize(pts[1] - pts[0])
        elif i == n - 1:
            tangent = normalize(pts[-1] - pts[-2])
        else:
            t0 = normalize(pts[i] - pts[i - 1])
            t1 = normalize(pts[i + 1] - pts[i])
            tangent = normalize(t0 + t1)
            if np.linalg.norm(tangent) < 1e-9:
                tangent = t1

        normal = np.array([-tangent[1], tangent[0]], dtype=float)
        normals.append(normal)

    return np.array(normals, dtype=float)


def triangulate_cap(indices):
    """
    Fan triangulation of a polygon described by indices.
    """
    faces = []
    for i in range(1, len(indices) - 1):
        faces.append([indices[0], indices[i], indices[i + 1]])
    return faces


def make_ribbon_wall(points, width, z_bottom, height, chamfer_enabled=False, chamfer_altitude=0.0):
    """
    Continuous ribbon wall.

    If chamfer_enabled:
      A 45-degree base chamfer expands the wall outward into the base.
      The parameter chamfer_altitude is the altitude from the right-angle
      corner to the chamfer face in the cross-section.

      For a 45-degree chamfer:
        leg = altitude * sqrt(2)

      So:
        horizontal expansion per side = leg
        vertical chamfer rise = leg
    """
    pts = np.asarray(points, dtype=float)
    if len(pts) < 2:
        return None

    normals = compute_path_normals(pts)
    n = len(pts)

    half_top = width / 2.0

    expand = 0.0
    chamfer_rise = 0.0
    if chamfer_enabled and chamfer_altitude > 0.0:
        expand = chamfer_altitude * math.sqrt(2.0)
        chamfer_rise = expand

    if chamfer_rise >= height:
        chamfer_rise = max(0.0, height - 1e-4)
        expand = chamfer_rise

    half_bottom = half_top + expand

    top_left_xy = pts + normals * half_top
    top_right_xy = pts - normals * half_top

    if chamfer_enabled and chamfer_altitude > 0.0 and chamfer_rise > 0.0:
        bottom_left_xy = pts + normals * half_bottom
        bottom_right_xy = pts - normals * half_bottom
    else:
        bottom_left_xy = top_left_xy.copy()
        bottom_right_xy = top_right_xy.copy()

    z0 = z_bottom
    z1 = z_bottom + chamfer_rise
    z2 = z_bottom + height

    vertices = []

    def add_strip(xy, z):
        start = len(vertices)
        for p in xy:
            vertices.append([p[0], p[1], z])
        return start

    # Bottom expanded footprint
    bL = add_strip(bottom_left_xy, z0)
    bR = add_strip(bottom_right_xy, z0)

    # Wall footprint after chamfer rise
    mL = add_strip(top_left_xy, z1)
    mR = add_strip(top_right_xy, z1)

    # Top of wall
    tL = add_strip(top_left_xy, z2)
    tR = add_strip(top_right_xy, z2)

    def idx(base, i):
        return base + i

    faces = []

    def add_quad(a, b, c, d):
        faces.append([a, b, c])
        faces.append([a, c, d])

    # Bottom face
    for i in range(n - 1):
        add_quad(
            idx(bL, i),
            idx(bL, i + 1),
            idx(bR, i + 1),
            idx(bR, i),
        )

    # Top face
    for i in range(n - 1):
        add_quad(
            idx(tL, i),
            idx(tR, i),
            idx(tR, i + 1),
            idx(tL, i + 1),
        )

    # Left outside face
    if chamfer_rise > 0.0:
        for i in range(n - 1):
            add_quad(
                idx(bL, i),
                idx(bL, i + 1),
                idx(mL, i + 1),
                idx(mL, i),
            )
    else:
        for i in range(n - 1):
            add_quad(
                idx(bL, i),
                idx(bL, i + 1),
                idx(tL, i + 1),
                idx(tL, i),
            )

    # Right outside face
    if chamfer_rise > 0.0:
        for i in range(n - 1):
            add_quad(
                idx(bR, i),
                idx(mR, i),
                idx(mR, i + 1),
                idx(bR, i + 1),
            )
    else:
        for i in range(n - 1):
            add_quad(
                idx(bR, i),
                idx(tR, i),
                idx(tR, i + 1),
                idx(bR, i + 1),
            )

    # Upper vertical left face
    if chamfer_rise > 0.0:
        for i in range(n - 1):
            add_quad(
                idx(mL, i),
                idx(mL, i + 1),
                idx(tL, i + 1),
                idx(tL, i),
            )

    # Upper vertical right face
    if chamfer_rise > 0.0:
        for i in range(n - 1):
            add_quad(
                idx(mR, i),
                idx(tR, i),
                idx(tR, i + 1),
                idx(mR, i + 1),
            )

    # Start cap
    if chamfer_rise > 0.0:
        start_poly = [
            idx(bL, 0),
            idx(bR, 0),
            idx(mR, 0),
            idx(tR, 0),
            idx(tL, 0),
            idx(mL, 0),
        ]
    else:
        start_poly = [
            idx(bL, 0),
            idx(bR, 0),
            idx(tR, 0),
            idx(tL, 0),
        ]
    faces.extend(triangulate_cap(start_poly))

    # End cap
    if chamfer_rise > 0.0:
        end_poly = [
            idx(bL, n - 1),
            idx(mL, n - 1),
            idx(tL, n - 1),
            idx(tR, n - 1),
            idx(mR, n - 1),
            idx(bR, n - 1),
        ]
    else:
        end_poly = [
            idx(bL, n - 1),
            idx(tL, n - 1),
            idx(tR, n - 1),
            idx(bR, n - 1),
        ]
    faces.extend(triangulate_cap(end_poly))

    mesh = trimesh.Trimesh(vertices=np.array(vertices), faces=np.array(faces), process=False)
    mesh.merge_vertices()

    if hasattr(mesh, "remove_duplicate_faces"):
        mesh.remove_duplicate_faces()
    if hasattr(mesh, "remove_degenerate_faces"):
        mesh.remove_degenerate_faces()
    if hasattr(mesh, "remove_unreferenced_vertices"):
        mesh.remove_unreferenced_vertices()

    return mesh


# ==========================================
# GUI App
# ==========================================
class App:
    def __init__(self, root):
        self.root = root
        self.root.title("LED Spiral Designer")

        self.params = {
            "outer_diameter": tk.DoubleVar(value=30.0),
            "base_thickness": tk.DoubleVar(value=1.6),
            "wall_height": tk.DoubleVar(value=2.0),
            "wall_thickness": tk.DoubleVar(value=0.5),
            "chamfer_altitude": tk.DoubleVar(value=0.10),
            "outer_wall_margin": tk.DoubleVar(value=0.5),
            "spiral_start_margin": tk.DoubleVar(value=1.6),
            "num_turns": tk.DoubleVar(value=2.2),
            "entry_slit_width": tk.DoubleVar(value=1.0),
            "entry_angle_deg": tk.DoubleVar(value=180.0),
            "center_radius": tk.DoubleVar(value=1.2),
        }

        self.chamfer_enabled = tk.BooleanVar(value=True)

        self._build_ui()
        self.update_plot()

    def _build_ui(self):
        main = tk.Frame(self.root)
        main.pack(fill="both", expand=True)

        controls = tk.Frame(main)
        controls.pack(side="left", fill="y", padx=10, pady=10)

        tk.Checkbutton(
            controls,
            text="Enable base chamfer",
            variable=self.chamfer_enabled,
            command=self.update_plot,
        ).pack(anchor="w", pady=(0, 8))

        scale_limits = {
            "outer_diameter": (10, 80, 0.1),
            "base_thickness": (0.4, 5.0, 0.1),
            "wall_height": (0.4, 6.0, 0.1),
            "wall_thickness": (0.2, 3.0, 0.05),
            "chamfer_altitude": (0.0, 1.5, 0.01),
            "outer_wall_margin": (0.0, 5.0, 0.05),
            "spiral_start_margin": (0.0, 8.0, 0.05),
            "num_turns": (0.5, 6.0, 0.05),
            "entry_slit_width": (0.2, 6.0, 0.05),
            "entry_angle_deg": (0.0, 360.0, 1.0),
            "center_radius": (0.2, 8.0, 0.05),
        }

        for name, var in self.params.items():
            row = tk.Frame(controls)
            row.pack(fill="x", pady=3)

            tk.Label(row, text=name, width=20, anchor="w").pack(side="left")

            lo, hi, inc = scale_limits[name]

            spin = tk.Spinbox(
                row,
                from_=lo,
                to=hi,
                increment=inc,
                textvariable=var,
                width=8,
                command=self.update_plot,
            )
            spin.pack(side="right")
            spin.bind("<Return>", lambda e: self.update_plot())
            spin.bind("<FocusOut>", lambda e: self.update_plot())

            scale = tk.Scale(
                controls,
                variable=var,
                from_=lo,
                to=hi,
                resolution=inc,
                orient="horizontal",
                command=lambda _=None: self.update_plot(),
            )
            scale.pack(fill="x")

        tk.Button(controls, text="Export STL", command=self.export_stl).pack(fill="x", pady=(10, 4))
        tk.Button(controls, text="Save PNG", command=self.save_png).pack(fill="x")

        self.fig, self.ax = plt.subplots(figsize=(7, 7))
        self.canvas = FigureCanvasTkAgg(self.fig, master=main)
        self.canvas.get_tk_widget().pack(side="right", fill="both", expand=True)

    def _get_values(self):
        return {k: v.get() for k, v in self.params.items()}

    def update_plot(self):
        p = self._get_values()

        od = p["outer_diameter"]
        wall_t = p["wall_thickness"]
        outer_r = od / 2.0

        expand = 0.0
        if self.chamfer_enabled.get():
            expand = p["chamfer_altitude"] * math.sqrt(2.0)

        self.ax.clear()
        self.ax.set_aspect("equal")
        self.ax.grid(True, alpha=0.25)

        self.ax.add_patch(Circle((0, 0), outer_r, fill=False, linewidth=1.5))

        outer_wall_center_r = outer_r - p["outer_wall_margin"] - wall_t / 2.0
        if outer_wall_center_r > wall_t:
            ring_path = make_ring_path(
                center_radius=outer_wall_center_r,
                slit_width=p["entry_slit_width"],
                slit_angle_deg=p["entry_angle_deg"],
                n=800,
            )
            normals = compute_path_normals(ring_path)

            ring_top_left = ring_path + normals * (wall_t / 2.0)
            ring_top_right = ring_path - normals * (wall_t / 2.0)
            ring_base_left = ring_path + normals * (wall_t / 2.0 + expand)
            ring_base_right = ring_path - normals * (wall_t / 2.0 + expand)

            self.ax.plot(ring_base_left[:, 0], ring_base_left[:, 1], linewidth=1.2)
            self.ax.plot(ring_base_right[:, 0], ring_base_right[:, 1], linewidth=1.2)
            if expand > 0.0:
                self.ax.plot(ring_top_left[:, 0], ring_top_left[:, 1], linestyle="--", linewidth=0.8)
                self.ax.plot(ring_top_right[:, 0], ring_top_right[:, 1], linestyle="--", linewidth=0.8)

        theta0 = math.radians(p["entry_angle_deg"])
        theta1 = theta0 + p["num_turns"] * 2.0 * math.pi
        r_start = outer_r - p["spiral_start_margin"]
        r_end = p["center_radius"]

        if r_start > r_end > 0:
            spiral_path = arch_spiral_points(r_start, r_end, theta0, theta1, 1200)
            normals = compute_path_normals(spiral_path)

            spiral_top_left = spiral_path + normals * (wall_t / 2.0)
            spiral_top_right = spiral_path - normals * (wall_t / 2.0)
            spiral_base_left = spiral_path + normals * (wall_t / 2.0 + expand)
            spiral_base_right = spiral_path - normals * (wall_t / 2.0 + expand)

            self.ax.plot(spiral_base_left[:, 0], spiral_base_left[:, 1], linewidth=1.5)
            self.ax.plot(spiral_base_right[:, 0], spiral_base_right[:, 1], linewidth=1.5)
            if expand > 0.0:
                self.ax.plot(spiral_top_left[:, 0], spiral_top_left[:, 1], linestyle="--", linewidth=0.8)
                self.ax.plot(spiral_top_right[:, 0], spiral_top_right[:, 1], linestyle="--", linewidth=0.8)

            if outer_wall_center_r > 0:
                entry_start = polar(outer_wall_center_r - wall_t / 2.0, theta0)
                entry_end = polar(r_start, theta0)
                entry_pts = np.array([entry_start, entry_end], dtype=float)
                entry_normals = compute_path_normals(entry_pts)

                entry_top_left = entry_pts + entry_normals * (wall_t / 2.0)
                entry_top_right = entry_pts - entry_normals * (wall_t / 2.0)
                entry_base_left = entry_pts + entry_normals * (wall_t / 2.0 + expand)
                entry_base_right = entry_pts - entry_normals * (wall_t / 2.0 + expand)

                self.ax.plot(entry_base_left[:, 0], entry_base_left[:, 1], linewidth=1.5)
                self.ax.plot(entry_base_right[:, 0], entry_base_right[:, 1], linewidth=1.5)
                if expand > 0.0:
                    self.ax.plot(entry_top_left[:, 0], entry_top_left[:, 1], linestyle="--", linewidth=0.8)
                    self.ax.plot(entry_top_right[:, 0], entry_top_right[:, 1], linestyle="--", linewidth=0.8)

        lim = outer_r + 4
        self.ax.set_xlim(-lim, lim)
        self.ax.set_ylim(-lim, lim)
        self.ax.set_title("Top View Preview")
        self.ax.set_xlabel("mm")
        self.ax.set_ylabel("mm")

        self.canvas.draw_idle()

    def export_stl(self):
        p = self._get_values()

        od = p["outer_diameter"]
        base_t = p["base_thickness"]
        wall_h = p["wall_height"]
        wall_t = p["wall_thickness"]
        chamfer_altitude = p["chamfer_altitude"]
        outer_r = od / 2.0

        outer_wall_center_r = outer_r - p["outer_wall_margin"] - wall_t / 2.0
        r_start = outer_r - p["spiral_start_margin"]
        r_end = p["center_radius"]

        if r_start <= r_end:
            messagebox.showerror("Invalid geometry", "Spiral start radius must be larger than center radius.")
            return

        if outer_wall_center_r <= wall_t:
            messagebox.showerror("Invalid geometry", "Outer wall radius is too small.")
            return

        chamfer_enabled = self.chamfer_enabled.get()
        chamfer_rise = chamfer_altitude * math.sqrt(2.0) if chamfer_enabled else 0.0
        if chamfer_enabled and chamfer_rise >= wall_h:
            messagebox.showerror(
                "Invalid geometry",
                "Chamfer is too large. The 45° rise (altitude × sqrt(2)) must be smaller than wall height."
            )
            return

        file = filedialog.asksaveasfilename(
            title="Export STL",
            defaultextension=".stl",
            filetypes=[("STL files", "*.stl")],
        )
        if not file:
            return

        base = trimesh.creation.cylinder(radius=outer_r, height=base_t, sections=180)
        base.apply_translation([0, 0, base_t / 2.0])

        overlap = min(0.08, base_t * 0.5)
        z_wall_bottom = base_t - overlap
        wall_total_h = wall_h + overlap

        meshes = [base]

        outer_wall_path = make_ring_path(
            center_radius=outer_wall_center_r,
            slit_width=p["entry_slit_width"],
            slit_angle_deg=p["entry_angle_deg"],
            n=800,
        )
        outer_wall = make_ribbon_wall(
            outer_wall_path,
            width=wall_t,
            z_bottom=z_wall_bottom,
            height=wall_total_h,
            chamfer_enabled=chamfer_enabled,
            chamfer_altitude=chamfer_altitude,
        )
        if outer_wall is not None:
            meshes.append(outer_wall)

        theta0 = math.radians(p["entry_angle_deg"])
        theta1 = theta0 + p["num_turns"] * 2.0 * math.pi
        spiral_pts = arch_spiral_points(r_start, r_end, theta0, theta1, 1400)
        spiral_wall = make_ribbon_wall(
            spiral_pts,
            width=wall_t,
            z_bottom=z_wall_bottom,
            height=wall_total_h,
            chamfer_enabled=chamfer_enabled,
            chamfer_altitude=chamfer_altitude,
        )
        if spiral_wall is not None:
            meshes.append(spiral_wall)

        entry_start = polar(outer_wall_center_r - wall_t / 2.0, theta0)
        entry_end = polar(r_start, theta0)
        entry_pts = np.array([entry_start, entry_end], dtype=float)
        entry_wall = make_ribbon_wall(
            entry_pts,
            width=wall_t,
            z_bottom=z_wall_bottom,
            height=wall_total_h,
            chamfer_enabled=chamfer_enabled,
            chamfer_altitude=chamfer_altitude,
        )
        if entry_wall is not None:
            meshes.append(entry_wall)

        mesh = trimesh.util.concatenate(meshes)
        mesh.merge_vertices()

        if hasattr(mesh, "remove_duplicate_faces"):
            mesh.remove_duplicate_faces()
        if hasattr(mesh, "remove_degenerate_faces"):
            mesh.remove_degenerate_faces()
        if hasattr(mesh, "remove_unreferenced_vertices"):
            mesh.remove_unreferenced_vertices()

        try:
            trimesh.repair.fix_normals(mesh)
        except Exception:
            pass

        mesh.export(file)
        messagebox.showinfo("Export complete", f"Saved STL:\n{file}")

    def save_png(self):
        file = filedialog.asksaveasfilename(
            title="Save Preview PNG",
            defaultextension=".png",
            filetypes=[("PNG files", "*.png")],
        )
        if not file:
            return

        self.fig.savefig(file, dpi=200, bbox_inches="tight")
        messagebox.showinfo("Saved", f"Saved preview:\n{file}")


if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()