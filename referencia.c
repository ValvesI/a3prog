// 1. Initialize Allegro
al_init();
al_install_keyboard();

// 2. Create a timer for 60 frames per second
ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

// 3. Create display
ALLEGRO_DISPLAY* display = al_create_display(800, 600);

// 4. Create Event Queue and register sources
ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
al_register_event_source(event_queue, al_get_display_event_source(display));
al_register_event_source(event_queue, al_get_timer_event_source(timer));

// 5. Start the timer
al_start_timer(timer);

bool running = true;
bool redraw = true;

// 6. Game Loop
while (running) {
    ALLEGRO_EVENT ev;
    al_wait_for_event(event_queue, &ev);

    if (ev.type == ALLEGRO_EVENT_TIMER) {
        // Update game logic here
        redraw = true;
    }
    else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
        running = false;
    }

    if (redraw && al_is_event_queue_empty(event_queue)) {
        redraw = false;
        
        // Render graphics
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_flip_display();
    }
}

// Clean up
al_destroy_timer(timer);
al_destroy_display(display);
al_destroy_event_queue(event_queue);