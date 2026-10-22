player_x  = 48
player_y  = 48
player_vx = 0.7
player_vy = 0.5

function player_update()
    player_x = player_x + player_vx
    player_y = player_y + player_vy

    if player_x < 2 then
        player_x = 2;  player_vx = -player_vx
    elseif player_x > 93 then
        player_x = 93; player_vx = -player_vx
    end
    if player_y < 2 then
        player_y = 2;  player_vy = -player_vy
    elseif player_y > 93 then
        player_y = 93; player_vy = -player_vy
    end
end

function player_draw()
    local px = math.floor(player_x)
    local py = math.floor(player_y)
    for dx = -1, 1 do
        for dy = -1, 1 do
            pset(px + dx, py + dy, 1)
        end
    end
end