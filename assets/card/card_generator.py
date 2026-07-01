import os
from PIL import Image, ImageDraw, ImageFont

# 1. SETUP CARTELLA E COLORI
os.makedirs("assets", exist_ok=True)

colori = {
    "rosso": (218, 41, 28, 255),
    "giallo": (255, 199, 44, 255),
    "verde": (0, 154, 68, 255),
    "blu": (0, 130, 202, 255)
}

WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)
OFFSET = 4 # Effetto ombra per profondità

# 2. CERCA IL FONT DI SISTEMA MASSICCIO SU WINDOWS
font_candidates = [
    "C:\\Windows\\Fonts\\ariblk.ttf",     # Arial Black
    "C:\\Windows\\Fonts\\impact.ttf",     # Impact
    "C:\\Windows\\Fonts\\trebucbd.ttf",   # Trebuchet Bold
    "C:\\Windows\\Fonts\\arialbd.ttf"     # Arial Bold
]

sys_font_path = None
for fc in font_candidates:
    if os.path.exists(fc):
        sys_font_path = fc
        break

def get_font(size):
    if sys_font_path:
        return ImageFont.truetype(sys_font_path, size)
    print("ATTENZIONE: Nessun font di sistema trovato. Verrà usato il font di default.")
    return ImageFont.load_default()

# --- FUNZIONE DEDICATA PER IL DISEGNO DELLE FRECCE INVERTI ---
def disegna_frecce_inverti(draw, cx, cy, color):
    r = 55 # Raggio dell'arco
    thick = 16 # Spessore linea
    
    # Freccia Superiore (Arco da sinistra a destra, punta a destra)
    draw.arc([(cx-r, cy-r), (cx+r, cy+r)], 190, 350, fill=color, width=thick)
    # Punta della freccia superiore
    px1, py1 = cx + r - 5, cy - 10
    draw.polygon([(px1-15, py1-15), (px1+15, py1), (px1-15, py1+15)], fill=color)
    
    # Freccia Inferiore (Arco da destra a sinistra, punta a sinistra)
    draw.arc([(cx-r, cy-r), (cx+r, cy+r)], 10, 170, fill=color, width=thick)
    # Punta della freccia inferiore
    px2, py2 = cx - r + 5, cy + 10
    draw.polygon([(px2+15, py2+15), (px2-15, py2), (px2+15, py2-15)], fill=color)

# 3. FUNZIONE DI CREAZIONE CARTA REALE CON GLI ANGOLI
def crea_carta_perfetta(nome_file, colore_bg, testo, action_type=None, is_jolly=False, is_dorso=False):
    W, H = 260, 400
    img = Image.new("RGBA", (W, H), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    
    # Bordo esterno bianco
    draw.rounded_rectangle([(0, 0), (W-1, H-1)], radius=24, fill=WHITE)
    # Sfondo interno colorato
    draw.rounded_rectangle([(14, 14), (W-15, H-15)], radius=16, fill=colore_bg)
    
    cx, cy = W // 2, H // 2
    
    # --- LIVELLO ELLISSE CENTRALE INCLINATA ---
    ell_layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ell_draw = ImageDraw.Draw(ell_layer)
    bbox_ellisse = [(25, 100), (W-25, H-100)] 
    
    if is_dorso:
        ell_draw.ellipse(bbox_ellisse, fill=(255, 199, 44, 255))
    elif is_jolly:
        # Se è il Jolly Semplice (senza scritte), NON disegniamo l'ellisse bianca sotto.
        # Disegniamo solo se è il +4 per dare risalto al testo.
        if len(testo) > 1:
            ell_draw.ellipse(bbox_ellisse, fill=WHITE)
            
        # Spicchi colorati per il Jolly
        ell_draw.pieslice(bbox_ellisse, 0, 90, fill=colori["rosso"])
        ell_draw.pieslice(bbox_ellisse, 90, 180, fill=colori["blu"])
        ell_draw.pieslice(bbox_ellisse, 180, 270, fill=colori["verde"])
        ell_draw.pieslice(bbox_ellisse, 270, 360, fill=colori["giallo"])
    else:
        ell_draw.ellipse(bbox_ellisse, fill=WHITE)
        
    ell_layer = ell_layer.rotate(15, resample=Image.BICUBIC)
    img.alpha_composite(ell_layer)
    draw = ImageDraw.Draw(img) # Ricollega il draw
    
    # --- SCRITTURA TESTI E SIMBOLI CON FONT REALE ---
    if is_dorso:
        f_dorso = get_font(75)
        draw.text((cx+OFFSET, cy+OFFSET), "UNO", font=f_dorso, fill=(100,0,0,255), anchor="mm")
        draw.text((cx, cy), "UNO", font=f_dorso, fill=colori["rosso"], anchor="mm")
        
    elif is_jolly:
        # Scriviamo il testo SOLO se è un +4 (evitiamo la stella o testi sul jolly semplice)
        if len(testo) > 1:
            txt_draw = testo
            f_jolly = get_font(85)
            draw.text((cx+OFFSET, cy+OFFSET), txt_draw, font=f_jolly, fill=BLACK, anchor="mm")
            draw.text((cx, cy), txt_draw, font=f_jolly, fill=WHITE, anchor="mm")
            
            # Simboli negli angoli per il +4
            f_corner = get_font(28)
            draw.text((38, 42), txt_draw, font=f_corner, fill=WHITE, anchor="mm")
            draw.text((W-38, H-42), txt_draw, font=f_corner, fill=WHITE, anchor="mm")
        
    elif action_type:
        color_simbolo = colore_bg
        txt_disp = ""
        
        if action_type == "+2":
            txt_disp = "+2"
            f_action = get_font(100)
            draw.text((cx+OFFSET, cy+OFFSET), txt_disp, font=f_action, fill=(40,40,40,150), anchor="mm")
            draw.text((cx, cy), txt_disp, font=f_action, fill=color_simbolo, anchor="mm")
        elif action_type == "salta":
            txt_disp = "Ø"
            f_action = get_font(140)
            draw.text((cx+OFFSET, cy+OFFSET), txt_disp, font=f_action, fill=(40,40,40,150), anchor="mm")
            draw.text((cx, cy), txt_disp, font=f_action, fill=color_simbolo, anchor="mm")
        elif action_type == "inverti":
            txt_disp = "⇄"
            # Disegniamo le frecce contrapposte graficamente al centro
            disegna_frecce_inverti(draw, cx, cy, color_simbolo)
            
        # Simboli negli angoli
        f_corner = get_font(30)
        draw.text((38, 42), txt_disp, font=f_corner, fill=WHITE, anchor="mm")
        draw.text((W-38, H-42), txt_disp, font=f_corner, fill=WHITE, anchor="mm")
        
    else:
        # Numeri standard (0-9)
        f_num = get_font(120) 
        draw.text((cx+OFFSET, cy+OFFSET), testo, font=f_num, fill=(40,40,40,150), anchor="mm")
        draw.text((cx, cy), testo, font=f_num, fill=colore_bg, anchor="mm")
        
        # Simboli negli angoli
        f_corner = get_font(32)
        draw.text((38, 42), testo, font=f_corner, fill=WHITE, anchor="mm")
        draw.text((W-38, H-42), testo, font=f_corner, fill=WHITE, anchor="mm")

    img.save(os.path.join("assets", nome_file))
    print(f"Generata carta perfetta: {nome_file}")

# 4. GENERAZIONE INTERO MAZZO
print("\n--- RIGENERAZIONE ASSET CON FONT DI SISTEMA WINDOWS ---")
if sys_font_path:
    print(f"Font ad alto impatto rilevato con successo: {sys_font_path}")

for nome_colore, cod_rgb in colori.items():
    for i in range(10):
        crea_carta_perfetta(f"{nome_colore}_{i}.png", cod_rgb, str(i))
        
for nome_colore, cod_rgb in colori.items():
    crea_carta_perfetta(f"{nome_colore}_salta.png", cod_rgb, "", action_type="salta")
    crea_carta_perfetta(f"{nome_colore}_inverti.png", cod_rgb, "", action_type="inverti")
    crea_carta_perfetta(f"{nome_colore}_piu2.png", cod_rgb, "+2", action_type="+2")

# Jolly semplice passiamo una stringa vuota per disattivare scritte/ellisse
crea_carta_perfetta("jolly.png", BLACK, "", is_jolly=True)
crea_carta_perfetta("jolly_piu4.png", BLACK, "+4", is_jolly=True)
crea_carta_perfetta("dorso.png", colori["rosso"], "UNO", is_dorso=True)

print("\n[COMPLETATO] Tutte le carte presentano ora i font reali e gli indici negli angoli!")