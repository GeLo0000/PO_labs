from locust import HttpUser, task, between

class WebsiteUser(HttpUser):
    host = "http://localhost:8080"
    wait_time = between(1, 2)  # Час між запитами

    @task
    def load_index(self):
        self.client.get("/")  # Тестує головну сторінку

    @task
    def load_second_page(self):
        self.client.get("/page2.html")  # Тестує другу сторінку
