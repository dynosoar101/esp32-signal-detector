import pygame
import math
import socket

WIDTH = 800
HEIGHT = 600

# Colors
white = (255, 255, 255)
navy = (10, 12, 18)
red = (255, 0, 0)
green = (0, 255, 0)
yellow = (255, 255, 0)
gray = (150, 150, 150)

pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
FPS = pygame.time.Clock()

pygame.display.set_caption("ESP32 Device Scanner")
menu_rect = pygame.Rect(WIDTH // 2 - 75, HEIGHT - 80, 150, 45)
ESP_X = WIDTH // 2
ESP_Y = HEIGHT - 150
color = yellow  # State color

# Networking setup
UDP_IP = "0.0.0.0"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(False)

# Target ESP32 address storage
esp_address = None
detected_devices = []

# Continuous Scan Variables
is_scanning = False
last_scan_request_time = 0
SCAN_INTERVAL_MS = 4000  # Automatically ask the ESP for an update every 4 seconds when menu is open

def draw_thick_arch(surface, color, center, inner_radius, outer_radius, start_deg, end_deg):
    cx, cy = center
    points = []
    start_rad = math.radians(start_deg)
    end_rad = math.radians(end_deg)
    segments = 45
    for i in range(segments + 1):
        angle = start_rad + (end_rad - start_rad) * (i / segments)
        points.append((cx + outer_radius * math.cos(angle), cy + outer_radius * math.sin(angle)))
    for i in range(segments, -1, -1):
        angle = start_rad + (end_rad - start_rad) * (i / segments)
        points.append((cx + inner_radius * math.cos(angle), cy + inner_radius * math.sin(angle)))
    pygame.draw.polygon(surface, color, points)

def draw_cone(x, y, color):
    center_point = (x, y)
    draw_thick_arch(screen, color, center_point, inner_radius=80,  outer_radius=84,  start_deg=210, end_deg=330)
    draw_thick_arch(screen, color, center_point, inner_radius=180, outer_radius=184, start_deg=210, end_deg=330)
    draw_thick_arch(screen, color, center_point, inner_radius=280, outer_radius=284, start_deg=210, end_deg=330)

def draw_screen(color):
    screen.fill(navy)
    pygame.draw.circle(screen, color, (ESP_X, ESP_Y), 15)
    draw_cone(ESP_X, ESP_Y, color)
    
    # Draw Menu Button (Highlights green if actively listening/looping)
    btn_color = green if is_scanning else white
    pygame.draw.rect(screen, btn_color, menu_rect, width=2, border_radius=12)
    font = pygame.font.SysFont("Arial", 20, bold=True)
    text = font.render("Devices", True, btn_color)
    screen.blit(text, (menu_rect.x + 38, menu_rect.y + 10))

    # --- DRAW DETECTED DEVICES LIST ---
    list_font = pygame.font.SysFont("Arial", 16)
    title_text = list_font.render("Scanned Devices (Live Stream):" if is_scanning else "Scanned Devices:", True, gray)
    screen.blit(title_text, (30, 30))
    
    y_offset = 60
    if not detected_devices:
        none_text = list_font.render("None loaded. Click 'Devices' to toggle continuous scan.", True, gray)
        screen.blit(none_text, (30, y_offset))
    else:
        # Cap list display to fit inside the viewport height smoothly
        for device in detected_devices[:20]:
            dev_text = list_font.render(f"• {device}", True, white)
            screen.blit(dev_text, (30, y_offset))
            y_offset += 24

    pygame.display.update()

while True:
    current_time = pygame.time.get_ticks()

    # If menu toggle is active, periodically request a new scan payload from the hardware
    if is_scanning and esp_address:
        if current_time - last_scan_request_time > SCAN_INTERVAL_MS:
            sock.sendto(b"GET_DEVICES", esp_address)
            last_scan_request_time = current_time

    try:
        data, addr = sock.recvfrom(4096)  
        message = data.decode('utf-8').strip()
        esp_address = addr  

        if message.startswith("LIST:"):
            devices_string = message.replace("LIST:", "")
            if devices_string:
                detected_devices = [d.strip() for d in devices_string.split(",")]
            else:
                detected_devices = ["No devices picked up in the airwaves"]
        else:
            sock.sendto(b"GUI_ACTIVE", addr)
            if message == "RED": color = red
            elif message == "GREEN": color = green
            elif message == "YELLOW": color = yellow
            
    except BlockingIOError:
        pass

    draw_screen(color)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            exit()
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1: 
                if menu_rect.collidepoint(event.pos):
                    if esp_address:
                        is_scanning = not is_scanning # Toggle status
                        if is_scanning:
                            print("Continuous scanner activated.")
                            sock.sendto(b"GET_DEVICES", esp_address)
                            last_scan_request_time = current_time
                            detected_devices = ["Initial network sweep active... please wait"]
                        else:
                            print("Continuous scanner deactivated.")
                            detected_devices = [] # Wipe out old results on close
                    else:
                        print("Error: Handshake incomplete. Wait for ESP32 packet to establish endpoint address.")
    
    FPS.tick(60)