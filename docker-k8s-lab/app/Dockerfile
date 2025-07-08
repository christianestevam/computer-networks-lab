FROM python:3.11-slim

WORKDIR /app

# Instala dependências
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copia código da aplicação
COPY app.py .

# Expõe porta
EXPOSE 5000

# Comando para iniciar a aplicação
CMD ["python", "app.py"]